TARGETS = quickcalld

CFLAGS = -Wall -Werror $(CCANFLAGS) -I./
LDLIBS = -lusb -lasound

OBJS = quickcall.o main.o lib.o hid.o audio.o

all: $(TARGETS)

quickcalld: $(OBJS) libccan.a
	$(LINK.c) -o $@ $^ $(LDLIBS)

go: all
	./quickcalld $$(udevadm trigger --verbose --subsystem-match=usb --attr-match=idVendor=046d --attr-match=idProduct=08d5)

sgo: all
	strace ./quickcalld $$(udevadm trigger --verbose --subsystem-match=usb --attr-match=idVendor=046d --attr-match=idProduct=08d5)

clean: ccanclean
	rm -f $(TARGETS)
	rm -f *.o *.d
	rm -f *~

include ccan/Makefile.ccan

-include $(OBJS:%.o=%.d)
