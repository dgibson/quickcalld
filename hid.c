#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <sys/ioctl.h>

#include <linux/types.h>
#include <linux/hiddev.h>

#include <usb.h>

#include <ccan/array_size/array_size.h>

#include "lib.h"
#include "quickcall.h"

static void set_leds(struct quickcall *qc, int red, int green, int blue)
{
	int rc;
	struct hiddev_usage_ref ref = {
		.report_type = HID_REPORT_TYPE_FEATURE,
		.report_id = HID_REPORT_ID_UNKNOWN,
		.usage_code = QUICKCALL_USAGE_LEDS,
	};
	struct hiddev_report_info ri = {
		.report_type = HID_REPORT_TYPE_FEATURE,
		.report_id = HID_REPORT_ID_FIRST,
	};

	ref.value = red | (green << 2) | (blue << 4);

	rc = ioctl(qc->hidfd, HIDIOCSUSAGE, &ref);
	if (rc)
		die("Error on HIDIOCSUSAGE: %s\n", strerror(errno));

	rc = ioctl(qc->hidfd, HIDIOCSREPORT, &ri);
	if (rc)
		die("Error on HIDIOCSREPORT: %s\n", strerror(errno));
}

static void update_leds(struct quickcall *qc)
{
	set_leds(qc, qc->mutestate * QUICKCALL_LED_ON,
		 !qc->mutestate * QUICKCALL_LED_ON, 0);
}

static void mute_btn(struct quickcall *qc, struct hiddev_event *ev, void *data)
{
	if (!ev->value)
		return;

	qc->mutestate = !qc->mutestate;
	update_leds(qc);
	quickcall_update_mute(qc);
}

static void volume_btn(struct quickcall *qc, struct hiddev_event *ev, void *data)
{
	int delta = (int)data;
	quickcall_update_volume(qc, delta);
}

static struct button_info {
	unsigned usage;
	char mask;
	void (*fn)(struct quickcall *qc, struct hiddev_event *ev,
		   void *data);
	void *data;
	const char *name;
} button_table[] = {
	{QUICKCALL_USAGE_CALL, 0x01, NULL, NULL, "CALL"},
	{QUICKCALL_USAGE_MUTE, 0x02, mute_btn, NULL, "MUTE"},
	{QUICKCALL_USAGE_HANGUP, 0x04, NULL, NULL, "HANGUP"},
	{QUICKCALL_USAGE_MIC, 0x08, NULL, NULL, "MIC SOCKET"},
	{QUICKCALL_USAGE_HEADPHONE, 0x10, NULL, NULL, "HEADPHONE SOCKET"},
	{QUICKCALL_USAGE_RIGHT, 0x20, volume_btn, (void *)(+1), "RIGHT"},
	{QUICKCALL_USAGE_LEFT, 0x40, volume_btn, (void *)(-1), "LEFT"},
	{QUICKCALL_USAGE_MYSTERY, 0x80,NULL, NULL, "MYSTERY"},
};

static void process_event(struct quickcall *qc, struct hiddev_event *ev)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(button_table); i++) {
		struct button_info *btn = &button_table[i];

		if (btn->usage == ev->hid) {
			signed int oldval = !!(qc->hidstate & btn->mask);

			if (oldval != ev->value) {
				if (ev->value)
					debug("%s pressed\n", btn->name);
				else
					debug("%s released\n", btn->name);

				qc->hidstate ^= btn->mask;

				if (btn->fn)
					btn->fn(qc, ev, btn->data);
			}

			return;
		}
	}
	debug("Received unknown usage 0x%08x (%d)\n", ev->hid, ev->value);
}

void quickcall_hidpoll(struct quickcall *qc)
{
	struct hiddev_event ev;
	int rc;

	update_leds(qc);

	while (1) {
		rc = read(qc->hidfd, &ev, sizeof(ev));
		if (rc < 0)
			die("Error reading from %s: %s\n",
			    qc->hiddev, strerror(errno));
		if (rc == 0)
			return; /* EOF */
		if (rc != sizeof(ev))
			die("Short read from %s\n", qc->hiddev);

		process_event(qc, &ev);
	}
}
