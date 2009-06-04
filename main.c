#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include <unistd.h>
#include <syslog.h>

#include <usb.h>

#include <ccan/daemonize/daemonize.h>

#include "lib.h"
#include "quickcall.h"

int main(int argc, char *argv[])
{
	int opt;
	FILE *logfile = stderr;
	struct quickcall *qc;
	const char *sysdir;

	log_init(stderr);

	while ((opt = getopt(argc, argv, "vl:")) != -1) {
		switch (opt) {
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
		default:
			exit(2);
		}
	}


	if (optind != (argc - 1))
		die("Usage: quickcalld [-v] [-l <logfile>] <sysfs dir>\n");

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

	if (!daemonize())
		die("Failed to daemonize: %s\n", strerror(errno));

	if (logfile == stderr)
		log_init(NULL); /* Switch logging to syslog */

	log_printf(LOG_NOTICE, "Running on Quickcall device at %s\n", sysdir);

	quickcall_hidpoll(qc);

	exit(0);
}
