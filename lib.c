#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include <syslog.h>

#include <ccan/talloc/talloc.h>
#include <ccan/grab_file/grab_file.h>

#include "lib.h"

static FILE *logfile;
int verbose = 0;

void log_init(FILE *f)
{
	logfile = f;

	if (!f)
		openlog("quickcalld", LOG_PID, LOG_DAEMON);
}

void log_vprintf(int priority, const char *fmt, va_list ap)
{	
	if (logfile)
		vfprintf(logfile, fmt, ap);
	else
		vsyslog(priority, fmt, ap);
}

void log_printf(int priority, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	log_vprintf(priority, fmt, ap);
	va_end(ap);
}

void debug(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	if (verbose)
		log_vprintf(LOG_DEBUG, fmt, ap);
	va_end(ap);
}

void __attribute__((noreturn)) __attribute__((format(printf, 1, 2))) die(char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	log_vprintf(LOG_ERR, fmt, ap);
	va_end(ap);
	exit(1);
}

int grab_file_strtol(const char *filename, int base, long *val)
{
	char *file;
	
	file = grab_file(NULL, filename, NULL);
	if (!file)
		return -1;

	*val = strtol(file, NULL, base);
	talloc_free(file);
	return 0;
}

int get_sys_attrib(const char *sysdir, const char *attrib, int base, long *val)
{
	char *name = talloc_asprintf(NULL, "%s/%s", sysdir, attrib);
	int rc;

	rc = grab_file_strtol(name, base, val);
	
	talloc_free(name);
	return rc;
}

