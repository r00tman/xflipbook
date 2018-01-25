#pragma once
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/extensions/XInput.h>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <string>


struct TabletEvent {
    int x, y, pressure;
};
class Tablet {
    XID _tabletID;
    Display *_display;
    XDevice *_tabletDev;
    int _mEventType;
 
public:
    Tablet(Display *display, XID id=-1)
        :_display(display), _tabletDev(nullptr) {

        _tabletID = id;
        if (id < 0) {
            auto devices = listDevices(display);
            printf("Number of devices is %lu\n", devices.size());
            _tabletID = 0;
            for (size_t i = 0; i < devices.size(); i++) {
                printf("%s (%lu)\n", devices[i].first.c_str(), devices[i].second);
            }
            printf("Select your tablet id from the list: ");
            scanf("%lu", &_tabletID);
        }
    }
    
    ~Tablet() {
        if (_tabletDev != nullptr) {
            close();
        }
    }

    static std::vector<std::pair<std::string, XID> > listDevices(Display *display) {
        int ndev;
        XDeviceInfo *devinfo;
        if ((devinfo = XListInputDevices(display, &ndev)) == NULL)
        {
            throw std::runtime_error("Failed to get list of devices\n");
        }

        std::vector<std::pair<std::string, XID> > devices;
        for (int i = 0; i < ndev; i++) {
            devices.push_back(make_pair(std::string(devinfo[i].name), devinfo[i].id));
        }

        XFreeDeviceList(devinfo);
        return devices;
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
