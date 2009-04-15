#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>

#include <linux/types.h>
#include <linux/hiddev.h>

#include <usb.h>

#include <ccan/array_size/array_size.h>

#include "lib.h"
#include "quickcall.h"

static struct {
	unsigned usage;
	char mask;
	const char *name;
} btntbl[] = {
	{QUICKCALL_USAGE_CALL, 0x01, "CALL"},
	{QUICKCALL_USAGE_MUTE, 0x02, "MUTE"},
	{QUICKCALL_USAGE_HANGUP, 0x04, "HANGUP"},
	{QUICKCALL_USAGE_LEFT, 0x08, "LEFT"},
	{QUICKCALL_USAGE_RIGHT, 0x10, "RIGHT"},
};

static void handle_button(unsigned usage, int val, const char *name)
{
	if (val)
		printf("%s pressed\n", name);
	else
		printf("%s released\n", name);


}

void quickcall_hidpoll(struct quickcall *qc)
{
	struct hiddev_event ev;
	int rc;
	int i;

	while (1) {
		rc = read(qc->hidfd, &ev, sizeof(ev));
		if (rc < 0)
			die("Error reading from %s: %s\n",
			    qc->hiddev, strerror(errno));
		if (rc == 0)
			return; /* EOF */
		if (rc != sizeof(ev))
			die("Short read from %s\n", qc->hiddev);

		for (i = 0; i < ARRAY_SIZE(btntbl); i++)
			if (btntbl[i].usage == ev.hid) {
				char mask = btntbl[i].mask;
				signed int oldval = !!(qc->hidstate & mask);

				if (oldval != ev.value) {
					qc->hidstate ^= mask;
					handle_button(ev.hid, ev.value,
						btntbl[i].name);
				}
			}
	}
}
