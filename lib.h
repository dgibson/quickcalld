#ifndef _LIB_H
#define _LIB_H

/* library stuff */
void __attribute((format(printf, 1, 2))) debug(const char *fmt, ...);
void __attribute__((noreturn)) __attribute__((format(printf, 1, 2))) die(char *fmt, ...);
int grab_file_strtol(const char *filename, int base, long *val);
int get_sys_attrib(const char *sysdir, const char *attrib, int base, long *val);

#endif /* _LIB_H */
