#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include <unistd.h>
#include <syslog.h>
#include <sys/wait.h>

#include <usb.h>

#include <ccan/daemonize/daemonize.h>

#include "lib.h"
#include "quickcall.h"

void udev_wait(FILE *logfile, int dont_detach)
{
	if (!dont_detach && !daemonize())
		die("Failed to daemonize: %s\n", strerror(errno));

	if (!dont_detach && logfile == stderr)
		log_init(NULL); /* Switch logging to syslog */

	/*
	 * FIXME.  This is wrong, bad, evil hack.
	 * 
	 * The right thing to do here is to wait for udev to complete
	 * processing all the deivces we need.  Unfortunately that
	 * would mean monitoring udev events, then checking to see
	 * which devices are there, then reading the monitored events
	 * until the 3 devices we need (usb interface, hiddev, and
	 * sound) all appear.  Or we can just do the wrong thing:
	 */
	sleep(1);
}

int main(int argc, char *argv[])
{
	int wait_for_udev = 0;
	int dont_detach = 0;
	int opt;
	FILE *logfile = stderr;
	struct quickcall *qc;
	const char *sysdir;

	log_init(stderr);

	while ((opt = getopt(argc, argv, "vl:ud")) != -1) {
		switch (opt) {
		case 'd':
			dont_detach = 1;
			break;
		case 'v':
			verbose = 1;
			break;
		case 'l':
			logfile = fopen(optarg, "a");
			if (!logfile)
				die("Couldn't open logfile %s: %s\n",
				    optarg, strerror(errno));
			log_init(logfile);
			break;
		case 'u':
			wait_for_udev = 1;
			break;
		default:
			exit(2);
		}
	}


	if (optind != (argc - 1))
		die("Usage: quickcalld [-u] [-v] [-d] [-l <logfile>] <sysfs dir>\n");

	if (wait_for_udev)
		udev_wait(logfile, dont_detach);

	/* Initialize libusb */
	usb_init();
	usb_find_busses();
    	usb_find_devices();

	sysdir = argv[optind];

	debug("Probing %s...\n", sysdir);

	qc = quickcall_probe(sysdir);
	if (!qc)
		die("Invoked on %s which is not a Quickcall device\n", sysdir);

	quickcall_open(qc);

	/* Daemonize if we didn't do so already */
	if (!wait_for_udev && !dont_detach && !daemonize())
		die("Failed to daemonize: %s\n", strerror(errno));

	if (!dont_detach && logfile == stderr)
		log_init(NULL); /* Switch logging to syslog */

	log_printf(LOG_NOTICE, "Running on Quickcall device at %s\n", sysdir);

	quickcall_hidpoll(qc);

	exit(0);
}
