#! /usr/bin/env python

import sys
import usb
import os

QC_VENDOR = 0x046d
QC_PRODUCT = 0x08d5

def is_quickcall(idvendor, idproduct):
    return (idvendor == QC_VENDOR) and (idproduct == QC_PRODUCT)

def getdesc(handle, type, index):
    l = handle.getDescriptor(type, index, 1)[0]
    return handle.getDescriptor(type, index, l)

def getstrdesc(handle, index):
    desc = getdesc(handle, usb.DT_STRING, index)
    ucs = ''.join([chr(x) for x in desc[2:]])
    return ucs.decode('utf-16')

CALL = 0x04
MUTE = 0x08
HANGUP = 0x10
ROT = 0x3

def checkbutton(name, mask, x, lastx):
    if (x & mask) and not (lastx & mask):
        print "%s pressed" % name
    elif not (x & mask) and (lastx & mask):
        print "%s released" % name

rights = [(0, 1), (1, 3), (3, 2), (2, 0)]

DEVTOHOST = 0x80
HOSTTODEV = 0x00

GET_REPORT = 0x01
SET_REPORT = 0x09

INPUT_REPORT = 0x0100
FEATURE_REPORT = 0x0300

def hidpoll(h, iface):
    h.claimInterface(iface)
    h.resetEndpoint(0x83)
    x = h.controlMsg(DEVTOHOST | usb.TYPE_CLASS | usb.RECIP_INTERFACE,
                     GET_REPORT, 2, INPUT_REPORT, iface)[0]
    try:
        y = h.interruptRead(0x83, 2)
    except usb.USBError:
        y = None
    val = 0
    while True:
        lastx = x
        x = h.controlMsg(DEVTOHOST | usb.TYPE_CLASS | usb.RECIP_INTERFACE,
                         GET_REPORT, 2, INPUT_REPORT, iface)[0]
        try:
            y = h.interruptRead(0x83, 2)
        except usb.USBError:
            y = None
        #if y:
        #    print "Interrupt: %s" % (y,)
        if (x != lastx):
            #print "Input report: %s" % (x,)
            checkbutton('CALL', CALL, x, lastx)
            checkbutton('MUTE', MUTE, x, lastx)
            checkbutton('HANGUP', HANGUP, x, lastx)
            if (lastx & ROT, x & ROT) in rights:
                print "LEFT"
                val = val - 1
            elif (x & ROT, lastx & ROT) in rights:
                print "RIGHT"
                val = val + 1
            val = val & 0x1ff
            print val >> 1
            h.controlMsg(HOSTTODEV | usb.TYPE_CLASS | usb.RECIP_INTERFACE,
                         SET_REPORT, [val >> 1], FEATURE_REPORT, iface)

MUTE_CONTROL = 0x0100
VOLUME_CONTROL = 0x0200

GET_CUR = 0x81
GET_MIN = 0x82
GET_MAX = 0x83
GET_RES = 0x84

def featureprint(h, iface, unit):
    dd.claimInterface(iface)
    x = h.controlMsg(DEVTOHOST | usb.TYPE_CLASS | usb.RECIP_INTERFACE,
                     GET_CUR, 1, MUTE_CONTROL, iface | unit<<8)
    if x[0]:
        mute = "Muted"
    else:
        mute = "Unmuted"

    vcur = h.controlMsg(DEVTOHOST | usb.TYPE_CLASS | usb.RECIP_INTERFACE,
                        GET_CUR, 2, VOLUME_CONTROL, iface | unit<<8)
    vcur = vcur[0] << 8 | vcur[1] 
    vmin = h.controlMsg(DEVTOHOST | usb.TYPE_CLASS | usb.RECIP_INTERFACE,
                        GET_MIN, 2, VOLUME_CONTROL, iface | unit<<8)
    vmin = vmin[0] << 8 | vmin[1] 
    vmax = h.controlMsg(DEVTOHOST | usb.TYPE_CLASS | usb.RECIP_INTERFACE,
                        GET_MAX, 2, VOLUME_CONTROL, iface | unit<<8)
    vmax = vmax[0] << 8 | vmax[1] 
    vres = h.controlMsg(DEVTOHOST | usb.TYPE_CLASS | usb.RECIP_INTERFACE,
                        GET_RES, 2, VOLUME_CONTROL, iface | unit<<8)
    vres = vres[0] << 8 | vres[1] 
    print "%s, %d - %d - %d (%d)" % (mute, vmin, vcur, vmax, vres)


def debug(msg):
    print >>sys.stderr, msg

def find_sysfs(b, d):
    bus = int(b.dirname)
    dev = int(d.filename)
    return "/sys/bus/usb/devices/%d-%d" % (bus, dev)

class QuickCallError(Exception):
    pass

class QuickCall(object):
    def __init__(self, bus, dev):
        if not is_quickcall(dev.idVendor, dev.idProduct):
            raise QuickCallError("Device is not a Logitech Quickcall!")

        busnum = int(bus.dirname)
        devnum = int(dev.filename)

        self.sysdir = "/sys/bus/usb/devices/%d-%d" % (busnum, devnum)

        debug("Quickcall on bus %d, device %d.  Sysfs dir is %s"
              % (busnum, devnum, self.sysdir))

        v = int(open(self.sysdir + '/idVendor').read(), 16)
        p = int(open(self.sysdir + '/idProduct').read(), 16)
        if not is_quickcall(v, p):
            raise QuickCallError("sysfs id does not match Logitech Quickcall")

        basename = os.listdir('%s/%d-%d:1.3/usb'
                              % (self.sysdir, busnum, devnum))[0]
        self.hiddevname = "/dev/usb/%s" % basename

        debug("Opening %s" % self.hiddevname)

        hidfd = os.open(self.hiddevname, os.O_RDWR)

        self.handle = dev.open()
        self.hw_init()


    def hw_init(self):
        print self.handle.controlMsg.__doc__
        self.handle.controlMsg(HOSTTODEV | usb.TYPE_VENDOR | usb.RECIP_INTERFACE,
                               0x82, [0x01], 0, 0)
        self.handle.controlMsg(DEVTOHOST | usb.TYPE_VENDOR | usb.RECIP_INTERFACE,
                               0x81, 2, 0, 0)

for nb, b in enumerate(usb.busses()):
    for nd, d in enumerate(b.devices):
        if is_quickcall(d.idVendor, d.idProduct):
            q = QuickCall(b, d)
