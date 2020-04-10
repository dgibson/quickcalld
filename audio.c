#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <errno.h>
#include <syslog.h>

#include "lib.h"
#include "quickcall.h"

static int get_alsa_ctl(struct quickcall *qc, int elem)
{
	snd_ctl_elem_value_t *elem_val;
	int val;
	int rc;

	snd_ctl_elem_value_alloca(&elem_val);
	snd_ctl_elem_value_set_numid(elem_val, elem);

	rc = snd_ctl_elem_read(qc->alsactl, elem_val);
	if (rc != 0) {
		log_printf(LOG_ERR,
			   "Error on snd_ctl_elem_read(), element %d: %s\n",
			   elem, strerror(errno));
		return 0;
	}


	switch (elem) {
	case QUICKCALL_ALSACTL_OUTPUT_MUTE:
	case QUICKCALL_ALSACTL_INPUT_MUTE:
		val = snd_ctl_elem_value_get_boolean(elem_val, 0);
		break;

	case QUICKCALL_ALSACTL_OUTPUT_VOL:
	case QUICKCALL_ALSACTL_INPUT_VOL:
		val = snd_ctl_elem_value_get_integer(elem_val, 0);
		break;
	default:
		assert(0);
	}

	return val;
}

static void set_alsa_ctl(struct quickcall *qc, int elem, int val)
{
	snd_ctl_elem_value_t *elem_val;
	int rc;

	snd_ctl_elem_value_alloca(&elem_val);
	snd_ctl_elem_value_set_numid(elem_val, elem);

	switch (elem) {
	case QUICKCALL_ALSACTL_OUTPUT_MUTE:
	case QUICKCALL_ALSACTL_INPUT_MUTE:
		snd_ctl_elem_value_set_boolean(elem_val, 0, val);
		break;

	case QUICKCALL_ALSACTL_OUTPUT_VOL:
	case QUICKCALL_ALSACTL_INPUT_VOL:
		snd_ctl_elem_value_set_integer(elem_val, 0, val);
		break;
	default:
		assert(0);
	}

	rc = snd_ctl_elem_write(qc->alsactl, elem_val);
	if (rc)
		log_printf(LOG_ERR, "Error on snd_ctl_elem_write(): %s\n",
			   strerror(errno));
}

void quickcall_update_mute(struct quickcall *qc)
{
	set_alsa_ctl(qc, QUICKCALL_ALSACTL_OUTPUT_MUTE, !qc->mutestate);
	set_alsa_ctl(qc, QUICKCALL_ALSACTL_INPUT_MUTE, !qc->mutestate);
}

void quickcall_update_volume(struct quickcall *qc, int delta)
{
	set_alsa_ctl(qc, QUICKCALL_ALSACTL_OUTPUT_VOL,
		     get_alsa_ctl(qc, QUICKCALL_ALSACTL_OUTPUT_VOL) + delta);
}

void dump_audio(struct quickcall *qc)
{
	snd_ctl_elem_value_t *val;
	int i;
	int rc;

	snd_ctl_elem_value_alloca(&val);

	for (i = 1; i <= 4; i++) {
		snd_ctl_elem_value_set_numid(val, i);
		rc = snd_ctl_elem_read(qc->alsactl, val);
		if (rc != 0)
			log_printf(LOG_ERR,
				   "Error on snd_ctl_elem_read(), element %d\n",
				   i);
		else if (i % 2) {
			debug("Element %d (boolean?): %s\n", i,
			      snd_ctl_elem_value_get_boolean(val, 0) ? "on" : "off");
		} else {
			debug("Element %d (integer?): %ld\n", i,
			      snd_ctl_elem_value_get_integer(val, 0));
		}
	}
}
