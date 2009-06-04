#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include <unistd.h>

#include <usb.h>

#include "lib.h"
#include "quickcall.h"

int main(int argc, char *argv[])
{
	int opt;
	FILE *logfile = stderr;

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

	quickcall_do(argv[optind]);

	exit(0);
}
