#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <glob.h>

#include <usb.h>

#include <ccan/talloc/talloc.h>
#include <ccan/grab_file/grab_file.h>
#include <ccan/str/str.h>

#include "lib.h"
#include "quickcall.h"

static struct usb_bus *find_libusb_bus(const char *sysdir)
{
	struct usb_bus *bus;
	long busnum;
	int rc;

	rc = get_sys_attrib(sysdir, "busnum", 10, &busnum);
	if (rc)
		die("Couldn't get busnum from %s: %s\n",
		    sysdir, strerror(errno));

	for (bus = usb_get_busses(); bus; bus = bus->next) {
		char *eptr;
		int num = strtol(bus->dirname, &eptr, 10);

		if (*eptr)
			die("Couldn't parse bus dirname \"%s\"\n", bus->dirname);

		if (busnum == num)
			return bus;
	}

	return NULL;
}

static struct usb_device *find_libusb_dev(const char *sysdir, struct usb_bus *bus)
{
	struct usb_device *dev;
	long devnum;
	int rc;

	rc = get_sys_attrib(sysdir, "devnum", 10, &devnum);
	if (rc)
		die("Couldn't get devnum from %s: %s\n",
		    sysdir, strerror(errno));

	for (dev = bus->devices; dev; dev = dev->next) {
		char *eptr;
		int num = strtol(dev->filename, &eptr, 10);
		if (*eptr)
			die("Couldn't parse dev filename \"%s\"\n",
			    dev->filename);

		if (devnum == num)
			return dev;
	}

	return NULL;
}

static int sysfs_is_quickcall(const char *sysdir, uint16_t vendor, uint16_t product)
{
	int rc;
	struct stat sb;
	long idv, idp;

	rc = stat(sysdir, &sb);
	if (rc != 0)
		die("Couldn't stat %s: %s\n", sysdir, strerror(errno));

	if (!S_ISDIR(sb.st_mode))
		die("%s is not a directory\n", sysdir);

	rc = get_sys_attrib(sysdir, "idVendor", 16, &idv);
	if (rc < 0)
		die("Couldn't get idVendor from %s: %s\n",
		    sysdir, strerror(errno));

	rc = get_sys_attrib(sysdir, "idProduct", 16, &idp);
	if (rc < 0)
		die("Couldn't get idProduct from %s: %s\n",
		    sysdir, strerror(errno));

	return (idv == vendor) && (idp == product);
}

static char *find_hiddev(const char *sysdir)
{
	char *pattern, *tmp, *hiddev;
	glob_t gglob;
	int rc;
	struct stat sb;

	tmp = strrchr(sysdir, '/');
	assert(tmp);

	pattern = talloc_asprintf(NULL, "%s/%s:1.3/usb/hiddev*",
				  sysdir, tmp + 1);

	rc = glob(pattern, GLOB_NOSORT, NULL, &gglob);
	if (rc != 0)
		die("glob(\"%s\") failed\n", pattern);

	if (gglob.gl_pathc != 1)
		die("%d matches for %s instead of expected 1\n",
		    gglob.gl_pathc, pattern);

	talloc_free(pattern);

	tmp = strrchr(gglob.gl_pathv[0], '/');
	assert(tmp);

	hiddev = talloc_asprintf(NULL, "/dev/usb/%s", tmp + 1);

	globfree(&gglob);

	rc = stat(hiddev, &sb);
	if (rc != 0)
		die("Couldn't stat %s: %s\n", hiddev, strerror(errno));

	if (!S_ISCHR(sb.st_mode))
		die("%s is not a character device\n", hiddev);

	return hiddev;
}

static char *find_alsadev(const char *sysdir)
{
	char *pattern, *tmp, *eptr, *ctldev;
	glob_t gglob;
	int rc;
	struct stat sb;
	int cardnum;

	tmp = strrchr(sysdir, '/');
	assert(tmp);

	pattern = talloc_asprintf(NULL, "%s/%s:1.0/sound/card**",
				  sysdir, tmp + 1);

	rc = glob(pattern, GLOB_NOSORT, NULL, &gglob);
	if (rc != 0)
		die("glob(\"%s\") failed\n", pattern);

	if (gglob.gl_pathc != 1)
		die("%d matches for %s instead of expected 1\n",
		    gglob.gl_pathc, pattern);

	talloc_free(pattern);

	tmp = strrchr(gglob.gl_pathv[0], '/');
	assert(tmp);
	tmp += 5; /* skip past "/card" */

	cardnum = strtol(tmp, &eptr, 10);
	if (*eptr)
		die("Couldn't parse ALSA card number from \"%s\"\n",
		    gglob.gl_pathv[0]);
	globfree(&gglob);

	/* Sanity check */
	ctldev = talloc_asprintf(NULL, "/dev/snd/controlC%d", cardnum);
	rc = stat(ctldev, &sb);
	if (rc != 0)
		die("Couldn't stat %s: %s\n", ctldev, strerror(errno));

	if (!S_ISCHR(sb.st_mode))
		die("%s is not a character device\n", ctldev);

	talloc_free(ctldev);

	return talloc_asprintf(NULL, "hw:%d", cardnum);
}

struct quickcall *quickcall_probe(const char *sysdir)
{
	struct usb_bus *bus;
	struct usb_device *dev;
	char *hiddev, *alsadev;
	struct quickcall *qc;

	debug("Probing for Quickcall at %s\n", sysdir);

	if (!sysfs_is_quickcall(sysdir, QUICKCALL_VENDORID, QUICKCALL_PRODUCTID))
		return NULL;

	bus = find_libusb_bus(sysdir);
	if (!bus)
		die("Couldn't locate bus for %s via libusb\n", sysdir);

	dev = find_libusb_dev(sysdir, bus);
	if (!dev)
		die("Couldn't locate dev for %s via libusb\n", sysdir);

	/* Sanity check */
	if ((dev->descriptor.idVendor != QUICKCALL_VENDORID)
	    || (dev->descriptor.idProduct != QUICKCALL_PRODUCTID))
		die("ID retreived from libusb (%04x:%04x) does not"
		    " match Quickcall (%04x:%04x)",
		    dev->descriptor.idVendor, dev->descriptor.idProduct,
		    QUICKCALL_VENDORID, QUICKCALL_PRODUCTID);

	debug("Quickcall located at %s\n", sysdir);

	hiddev = find_hiddev(sysdir);

	debug("Quickcall hiddev is %s\n", hiddev);

	alsadev = find_alsadev(sysdir);
	debug("ALSA device is %s\n", alsadev);

	qc = talloc_zero(NULL, struct quickcall);
	if (!qc)
		die("Couldn't allocate memory\n");

	qc->sysdir = talloc_strdup(qc, sysdir);
	qc->bus = bus;
	qc->dev = dev;
	qc->hiddev = hiddev;
	talloc_steal(qc, hiddev);
	qc->alsadev = alsadev;
	talloc_steal(qc, alsadev);
	
	return qc;
}

void quickcall_open(struct quickcall *qc)
{
	char outbuf = 0x01;
	char inbuf[2];
	int rc;

	qc->handle = usb_open(qc->dev);
	if (!qc->handle)
		die("Couldn't usb_open(): %s\n", strerror(errno));

	debug("Opened Quickcall libusb handle\n");

	qc->hidfd = open(qc->hiddev, O_RDWR);
	if (qc->hidfd < 0)
		die("Couldn't open %s: %s\n", qc->hiddev, strerror(errno));
	debug("Opened Quickcall hiddev (fd=%d)\n", qc->hidfd);

	rc = snd_ctl_open(&qc->alsactl, qc->alsadev, 0);
	if (rc != 0)
		die("Error on snd_ctl_open(): %s\n", strerror(errno));

	/* Initialize the hardware */
	rc = usb_control_msg(qc->handle,
			     USB_ENDPOINT_OUT | USB_TYPE_VENDOR | USB_RECIP_INTERFACE,
			     0x82, 0, 0, &outbuf, sizeof(outbuf), 100);
	if (rc < 0)
		die("Error sending control message 0x82: %s\n", strerror(errno));
	if (rc != sizeof(outbuf))
		die("Short write on initialization request 0x82\n");

	rc = usb_control_msg(qc->handle,
			     USB_ENDPOINT_IN | USB_TYPE_VENDOR | USB_RECIP_INTERFACE,
			     0x81, 0, 0, inbuf, sizeof(inbuf), 100);
	if (rc < 0)
		die("Error sending control message 0x81: %s\n", strerror(errno));
	if (rc != sizeof(inbuf))
		die("Short read on initialization request 0x81\n");

	debug("Hardware initialization complete (0x81 -> 0x%02x,0x%02x)\n",
	      inbuf[0], inbuf[1]);
	
}
