PREFIX = /usr/local
INSTALLROOT = 

QUICKCALLD = $(PREFIX)/sbin/quickcalld
RULESFILE = /etc/udev/rules.d/85-$(RULESTEMPLATE)

CFLAGS = -O2 -Wall $(CCANFLAGS) -I./
LDLIBS = -lusb -lasound

TARGETS = quickcalld
OBJS = quickcall.o main.o lib.o hid.o audio.o
RULESTEMPLATE = quickcall.rules

all: $(TARGETS)

quickcalld: $(OBJS) libccan.a
	$(LINK.c) -o $@ $^ $(LDLIBS)

install: all
	install -m 755 quickcalld $(INSTALLROOT)$(QUICKCALLD)
	sed 's!@QUICKCALLD@!$(QUICKCALLD)!' $(RULESTEMPLATE) > $(INSTALLROOT)$(RULESFILE)

uninstall:
	rm -f $(INSTALLROOT)$(QUICKCALLD) $(INSTALLROOT)$(RULESFILE)

go: all
	./quickcalld $$(udevadm trigger --verbose --subsystem-match=usb --attr-match=idVendor=046d --attr-match=idProduct=08d5)

clean: ccanclean
	rm -f $(TARGETS)
	rm -f *.o *.d
	rm -f *~

include ccan/Makefile.ccan

-include $(OBJS:%.o=%.d)
