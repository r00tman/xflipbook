#pragma once
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/extensions/XInput.h>
#include <cstdio>
#include <cstdlib>
#include <iostream>


struct TabletEvent {
    int x, y, pressure;
};
class Tablet {
    XID _tabletID;
    Display *_display;
    XDevice *_tabletDev;
    int _mEventType;
 
public:
    Tablet(Display *display, const char *name="Wacom Co.,Ltd. Pen and multitouch sensor Pen stylus")
        :_display(display), _tabletDev(nullptr) {
        int ndev;
        XDeviceInfo *devinfo;
        if ((devinfo = XListInputDevices(_display, &ndev)) == NULL)
        {
            throw std::runtime_error("Failed to get list of devices\n");
        }
        printf("Number of devices is %d\n", ndev);

        _tabletID = 0;
        for (int i = 0; i < ndev; i++)
        {
            printf("%d. %s (%lu)\n", i, devinfo[i].name, devinfo[i].id);
            if (strcmp(devinfo[i].name, name) == 0)
            {
                _tabletID = devinfo[i].id;
            }
        }
        XFreeDeviceList(devinfo);
        if (_tabletID == 0) {
            throw std::runtime_error("Can't find tablet");
        }
    }
    
    ~Tablet() {
        if (_tabletDev != nullptr) {
            close();
        }
    }

    void open(int screen, Window window) {
        _tabletDev = XOpenDevice(this->_display, _tabletID);
        int numtypes = 1;
        XEventClass EClist[10];
        /* bizzaro macro, to SET arg 2 and 3. Grrr. */
        DeviceMotionNotify(_tabletDev, _mEventType, EClist[0]);
        printf("DEBUG: motioneventtype is supposedly %d\n", _mEventType);
        XSelectExtensionEvent(_display, window, EClist, numtypes);
    }

    void close() {
        XCloseDevice(_display, _tabletDev);
    }

    bool eventOf(XEvent *event) {
        return event->type == _mEventType;
    }

    TabletEvent parse(XEvent *event) {
        auto ev = reinterpret_cast<XDeviceMotionEvent*>(event);
        return TabletEvent{ev->axis_data[0], ev->axis_data[1], ev->axis_data[2]};
    }
};
