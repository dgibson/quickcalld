#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include <usb.h>

#include "lib.h"
#include "quickcall.h"

int main(int argc, char *argv[])
{
	usb_init();
	usb_find_busses();
    	usb_find_devices();

	if (argc != 2)
		die("Usage: quickcalld <sysfs dir>\n");

	quickcall_do(argv[1]);

	exit(0);
}
