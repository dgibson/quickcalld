#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>

#include <usb.h>

#include "lib.h"
#include "quickcall.h"

void dump_audio(struct quickcall *qc)
{
	uint16_t val;
	int rc;

	rc = usb_control_msg(qc->handle,
			     USB_ENDPOINT_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE,
			     0x81, 0x200, 2 << 8, (char *)&val, sizeof(val), 100);
	if (rc < 0)
		die("Error on control msg: %s\n", strerror(errno));

	printf("0x81 -> 0x%04x\n", val);

	rc = usb_control_msg(qc->handle,
			     USB_ENDPOINT_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE,
			     0x82, 0x200, 2 << 8, (char *)&val, sizeof(val), 100);
	if (rc < 0)
		die("Error on control msg: %s\n", strerror(errno));

	printf("0x82 -> 0x%04x\n", val);
	rc = usb_control_msg(qc->handle,
			     USB_ENDPOINT_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE,
			     0x83, 0x200, 2 << 8, (char *)&val, sizeof(val), 100);
	if (rc < 0)
		die("Error on control msg: %s\n", strerror(errno));

	printf("0x83 -> 0x%04x\n", val);
}

