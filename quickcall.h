#ifndef _QUICKCALL_H
#define _QUICKCALL_H

#include <usb.h>
#include <alsa/asoundlib.h>
#include <linux/types.h>
#include <linux/hiddev.h>

#define SYSFS_PATH "/sys"

#define QUICKCALL_VENDORID	0x046d
#define QUICKCALL_PRODUCTID	0x08d5

enum quickcall_button {
	QUICKCALL_BTN_CALL = 0,
	QUICKCALL_BTN_MUTE,
	QUICKCALL_BTN_HANGUP,
	QUICKCALL_BTN_LEFT,
	QUICKCALL_BTN_RIGHT,
	QUICKCALL_BTN_NUM,	
};

struct quickcall {
	struct usb_bus *bus;
	struct usb_device *dev;
	usb_dev_handle *handle;

	char *sysdir;

	char *hiddev;
	int hidfd;
	char hidstate;

	char *alsadev;
	snd_ctl_t *alsactl;

	int mutestate;

};

#define QUICKCALL_USAGE_CALL		0xff000071
#define QUICKCALL_USAGE_MUTE		0xff000072
#define QUICKCALL_USAGE_HANGUP		0xff000073
#define QUICKCALL_USAGE_MIC		0xff000074
#define QUICKCALL_USAGE_HEADPHONE	0xff000075
#define QUICKCALL_USAGE_RIGHT		0xff000076
#define QUICKCALL_USAGE_LEFT		0xff000077
#define QUICKCALL_USAGE_MYSTERY		0xff0000cc

#define QUICKCALL_USAGE_LEDS		0xff000001

#define QUICKCALL_LED_OFF	0
#define QUICKCALL_LED_SLOWBLINK	1
#define QUICKCALL_LED_FASTBLINK	2
#define QUICKCALL_LED_ON	3

#define QUICKCALL_ALSACTL_OUTPUT_MUTE	1
#define QUICKCALL_ALSACTL_OUTPUT_VOL	2
#define QUICKCALL_ALSACTL_INPUT_MUTE	3
#define QUICKCALL_ALSACTL_INPUT_VOL	4

struct quickcall *quickcall_probe(const char *sysdir);
void quickcall_open(struct quickcall *qc);
void quickcall_hidpoll(struct quickcall *qc);

void quickcall_update_mute(struct quickcall *qc);
void quickcall_update_volume(struct quickcall *qc, int delta);

#endif /* _QUICKCALL_H */
