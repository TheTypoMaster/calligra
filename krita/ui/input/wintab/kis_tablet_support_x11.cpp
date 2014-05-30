/*
 *  Copyright (c) 2013 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "kis_tablet_support_x11.h"

#include <QDesktopWidget>
#include <QApplication>
#include <QWidget>
#include <QX11Info>

#include "kis_debug.h"
#include "kis_config.h"
#include <input/kis_tablet_event.h>
#include "kis_tablet_support.h"
#include "wacomcfg.h"

#include <input/kis_tablet_debugger.h>


/**
 * This is an analog of a Qt's variable qt_tabletChokeMouse.  It is
 * intended to block Mouse events after any accepted Tablet event. In
 * Qt it is available on X11 only, so we won't extend this behavior on
 * Windows.
 */
bool kis_tabletChokeMouse = false;

/**
 * This variable is true when at least one of our tablet is a Evdev
 * one. In such a case we need to request the extension events from
 * the server manually
 */
bool kis_haveEvdevTablets = false;

// from include/Xwacom.h
#  define XWACOM_PARAM_TOOLID 322
#  define XWACOM_PARAM_TOOLSERIAL 323

typedef WACOMCONFIG * (*PtrWacomConfigInit) (Display*, WACOMERRORFUNC);
typedef WACOMDEVICE * (*PtrWacomConfigOpenDevice) (WACOMCONFIG*, const char*);
typedef int *(*PtrWacomConfigGetRawParam) (WACOMDEVICE*, int, int*, int, unsigned*);
typedef int (*PtrWacomConfigCloseDevice) (WACOMDEVICE *);
typedef void (*PtrWacomConfigTerm) (WACOMCONFIG *);

static PtrWacomConfigInit ptrWacomConfigInit = 0;
static PtrWacomConfigOpenDevice ptrWacomConfigOpenDevice = 0;
static PtrWacomConfigGetRawParam ptrWacomConfigGetRawParam = 0;
static PtrWacomConfigCloseDevice ptrWacomConfigCloseDevice = 0;
static PtrWacomConfigTerm ptrWacomConfigTerm = 0;
Q_GLOBAL_STATIC(QByteArray, wacomDeviceName)

// link Xinput statically
#define XINPUT_LOAD(symbol) symbol

typedef int (*PtrXCloseDevice)(Display *, XDevice *);
typedef XDeviceInfo* (*PtrXListInputDevices)(Display *, int *);
typedef XDevice* (*PtrXOpenDevice)(Display *, XID);
typedef void (*PtrXFreeDeviceList)(XDeviceInfo *);
typedef int (*PtrXSelectExtensionEvent)(Display *, Window, XEventClass *, int);

struct KisX11Data
{
    Display *display;

    bool use_xinput;
    int xinput_major;
    int xinput_eventbase;
    int xinput_errorbase;

    PtrXCloseDevice ptrXCloseDevice;
    PtrXListInputDevices ptrXListInputDevices;
    PtrXOpenDevice ptrXOpenDevice;
    PtrXFreeDeviceList ptrXFreeDeviceList;
    PtrXSelectExtensionEvent ptrXSelectExtensionEvent;

    /* Warning: if you modify this list, modify the names of atoms in kis_x11_atomnames as well! */
    enum X11Atom {
        XWacomStylus,
        XWacomCursor,
        XWacomEraser,

        XTabletStylus,
        XTabletEraser,

        XInputTablet,

        XInputKeyboard,

        AxisLabels,
        ATOM,

        AbsX,
        AbsY,
        AbsPressure,
        AbsTiltX,
        AbsTiltY,

        WacomTouch,

        NPredefinedAtoms,
        NAtoms = NPredefinedAtoms
    };
    Atom atoms[NAtoms];
};

/* Warning: if you modify this string, modify the list of atoms in KisX11Data as well! */
static const char * kis_x11_atomnames = {
    // Wacom old. (before version 0.10)
    "Wacom Stylus\0"
    "Wacom Cursor\0"
    "Wacom Eraser\0"

    // Tablet
    "STYLUS\0"
    "ERASER\0"

    // XInput tablet device
    "TABLET\0"

    // Really "nice" Aiptek devices reporting they are a keyboard
    "KEYBOARD\0"

    // Evdev property that report the assignment of axes
    "Axis Labels\0"
    "ATOM\0"

    // Types of axes reported by evdev
    "Abs X\0"
    "Abs Y\0"
    "Abs Pressure\0"
    "Abs Tilt X\0"
    "Abs Tilt Y\0"

    // Touch capabilities reported by Wacom Intuos tablets
    "TOUCH\0"

};

KisX11Data *kis_x11Data = 0;

#define KIS_ATOM(x) kis_x11Data->atoms[KisX11Data::x]
#define KIS_X11 kis_x11Data

static void kis_x11_create_intern_atoms()
{
    const char *names[KisX11Data::NAtoms];
    const char *ptr = kis_x11_atomnames;

    int i = 0;
    while (*ptr) {
        names[i++] = ptr;
        while (*ptr)
            ++ptr;
        ++ptr;
    }

    Q_ASSERT(i == KisX11Data::NPredefinedAtoms);
    Q_ASSERT(i == KisX11Data::NAtoms);

#if defined(XlibSpecificationRelease) && (XlibSpecificationRelease >= 6)
    XInternAtoms(KIS_X11->display, (char **)names, i, False, KIS_X11->atoms);
#else
    for (i = 0; i < KisX11Data::NAtoms; ++i)
        KIS_X11->atoms[i] = XInternAtom(KIS_X11->display, (char *)names[i], False);
#endif
}

void QTabletDeviceData::SavedAxesData::tryFetchAxesMapping(XDevice *dev)
{
    Atom propertyType;
    int propertyFormat;
    unsigned long nitems;
    unsigned long bytes_after;
    unsigned char *prop;

    int result = XGetDeviceProperty(KIS_X11->display, dev,
                                    KIS_ATOM(AxisLabels),
                                    0, 16, false,
                                    AnyPropertyType,
                                    &propertyType,
                                    &propertyFormat,
                                    &nitems,
                                    &bytes_after,
                                    &prop);

    if (result == Success && propertyType > 0) {

        if (KisTabletDebugger::instance()->initializationDebugEnabled()) {
            qDebug() << "# Building tablet axes remapping table:";
        }

        QVector<AxesIndexes> axesMap(NAxes, Unused);

        for (unsigned int axisIndex = 0; axisIndex < nitems; axisIndex++) {
            Atom currentAxisName = ((Atom*)prop)[axisIndex];

            AxesIndexes mappedIndex =
                Unused;

            if (currentAxisName == KIS_ATOM(AbsX)) {
                mappedIndex = XCoord;
            } else if (currentAxisName == KIS_ATOM(AbsY)) {
                mappedIndex = YCoord;
            } else if (currentAxisName == KIS_ATOM(AbsPressure)) {
                mappedIndex = Pressure;
            } else if (currentAxisName == KIS_ATOM(AbsTiltX)) {
                mappedIndex = XTilt;
            } else if (currentAxisName == KIS_ATOM(AbsTiltY)) {
                mappedIndex = YTilt;
            }

            axesMap[axisIndex] = mappedIndex;

            if (KisTabletDebugger::instance()->initializationDebugEnabled()) {
                qDebug() << XGetAtomName(KIS_X11->display, currentAxisName)
                         << axisIndex << "->" << mappedIndex;
            }
        }

        this->setAxesMap(axesMap);
    }
}

void kis_x11_init_tablet()
{
    KisConfig cfg;
    bool disableTouchOnCanvas = cfg.disableTouchOnCanvas();

    // TODO: free this structure on exit
    KIS_X11 = new KisX11Data;
    KIS_X11->display = QX11Info::display();

    kis_x11_create_intern_atoms();

    // XInputExtension
    KIS_X11->use_xinput = false;
    KIS_X11->xinput_major = 0;
    KIS_X11->xinput_eventbase = 0;
    KIS_X11->xinput_errorbase = 0;

    // See if Xinput is supported on the connected display
    KIS_X11->ptrXCloseDevice = 0;
    KIS_X11->ptrXListInputDevices = 0;
    KIS_X11->ptrXOpenDevice = 0;
    KIS_X11->ptrXFreeDeviceList = 0;
    KIS_X11->ptrXSelectExtensionEvent = 0;
    KIS_X11->use_xinput = XQueryExtension(KIS_X11->display, "XInputExtension", &KIS_X11->xinput_major,
                                          &KIS_X11->xinput_eventbase, &KIS_X11->xinput_errorbase);
    if (KIS_X11->use_xinput) {
        KIS_X11->ptrXCloseDevice = XINPUT_LOAD(XCloseDevice);
        KIS_X11->ptrXListInputDevices = XINPUT_LOAD(XListInputDevices);
        KIS_X11->ptrXOpenDevice = XINPUT_LOAD(XOpenDevice);
        KIS_X11->ptrXFreeDeviceList = XINPUT_LOAD(XFreeDeviceList);
        KIS_X11->ptrXSelectExtensionEvent = XINPUT_LOAD(XSelectExtensionEvent);
    }

    if (KIS_X11->use_xinput) {
        int ndev,
            i,
            j;
        bool gotStylus,
            gotEraser;
        XDeviceInfo *devices = 0, *devs;
        XInputClassInfo *ip;
        XAnyClassPtr any;
        XValuatorInfoPtr v;
        XAxisInfoPtr a;
        XDevice *dev = 0;

        bool needCheckIfItIsReallyATablet;
        bool touchWacomTabletWorkaround;

        if (KIS_X11->ptrXListInputDevices) {
            devices = KIS_X11->ptrXListInputDevices(KIS_X11->display, &ndev);
            if (!devices)
                qWarning("QApplication: Failed to get list of tablet devices");
        }
        if (!devices)
            ndev = -1;

        QTabletEvent::TabletDevice deviceType;
        for (devs = devices, i = 0; i < ndev && devs; i++, devs++) {
            dev = 0;
            deviceType = QTabletEvent::NoDevice;
            gotStylus = false;
            gotEraser = false;
            needCheckIfItIsReallyATablet = false;
            touchWacomTabletWorkaround = false;

#if defined(Q_OS_IRIX)
#else


                if (devs->type == KIS_ATOM(XWacomStylus) || devs->type == KIS_ATOM(XTabletStylus) ||devs->type == KIS_ATOM(XInputTablet)) {
                    if (devs->type == KIS_ATOM(XInputTablet)) {
                        kis_haveEvdevTablets = true;
                    }
                    deviceType = QTabletEvent::Stylus;
                    if (wacomDeviceName()->isEmpty())
                        wacomDeviceName()->append(devs->name);
                    gotStylus = true;
                } else if (devs->type == KIS_ATOM(XWacomEraser) || devs->type == KIS_ATOM(XTabletEraser)) {
                    deviceType = QTabletEvent::XFreeEraser;
                    gotEraser = true;
                } else if (devs->type == KIS_ATOM(XInputKeyboard) && QString(devs->name) == "Aiptek") {
                    /**
                     * Some really "nice" tablets (more precisely,
                     * Genius G-Pen 510 (aiptek driver)) report that
                     * they are a "keyboard". Well, we cannot convince
                     * them that they are not, so just check if this
                     * "keyboard" has motion and proximity events. If
                     * it looks like a duck... :)
                     */
                    kis_haveEvdevTablets = true;
                    deviceType = QTabletEvent::Stylus;
                    gotStylus = true;
                    needCheckIfItIsReallyATablet = true;
                } else if (disableTouchOnCanvas &&
                           devs->type == KIS_ATOM(WacomTouch) &&
                           QString(devs->name).contains("Wacom")) {

                    kis_haveEvdevTablets = true;
                    deviceType = QTabletEvent::Stylus;
                    gotStylus = true;
                    touchWacomTabletWorkaround = true;
                }

#endif
            if (deviceType == QTabletEvent::NoDevice)
                continue;

            if (gotStylus || gotEraser) {
                if (KIS_X11->ptrXOpenDevice)
                    dev = KIS_X11->ptrXOpenDevice(KIS_X11->display, devs->id);

                if (!dev)
                    continue;

                QTabletDeviceData device_data;
                device_data.deviceType = deviceType;
                device_data.eventCount = 0;
                device_data.device = dev;
                device_data.xinput_motion = -1;
                device_data.xinput_key_press = -1;
                device_data.xinput_key_release = -1;
                device_data.xinput_button_press = -1;
                device_data.xinput_button_release = -1;
                device_data.xinput_proximity_in = -1;
                device_data.xinput_proximity_out = -1;
                device_data.isTouchWacomTablet = touchWacomTabletWorkaround;
                //device_data.widgetToGetPress = 0;

                if (dev->num_classes > 0) {
                    for (ip = dev->classes, j = 0; j < dev->num_classes;
                         ip++, j++) {
                        switch (ip->input_class) {
                        case KeyClass:
                            DeviceKeyPress(dev, device_data.xinput_key_press,
                                           device_data.eventList[device_data.eventCount]);
                            if (device_data.eventList[device_data.eventCount])
                                ++device_data.eventCount;
                            DeviceKeyRelease(dev, device_data.xinput_key_release,
                                             device_data.eventList[device_data.eventCount]);
                            if (device_data.eventList[device_data.eventCount])
                                ++device_data.eventCount;
                            break;
                        case ButtonClass:
                            DeviceButtonPress(dev, device_data.xinput_button_press,
                                              device_data.eventList[device_data.eventCount]);
                            if (device_data.eventList[device_data.eventCount])
                                ++device_data.eventCount;
                            DeviceButtonRelease(dev, device_data.xinput_button_release,
                                                device_data.eventList[device_data.eventCount]);
                            if (device_data.eventList[device_data.eventCount])
                                ++device_data.eventCount;
                            break;
                        case ValuatorClass:
                            // I'm only going to be interested in motion when the
                            // stylus is already down anyway!
                            DeviceMotionNotify(dev, device_data.xinput_motion,
                                               device_data.eventList[device_data.eventCount]);
                            if (device_data.eventList[device_data.eventCount])
                                ++device_data.eventCount;
                            ProximityIn(dev, device_data.xinput_proximity_in, device_data.eventList[device_data.eventCount]);
                            if (device_data.eventList[device_data.eventCount])
                                ++device_data.eventCount;
                            ProximityOut(dev, device_data.xinput_proximity_out, device_data.eventList[device_data.eventCount]);
                            if (device_data.eventList[device_data.eventCount])
                                ++device_data.eventCount;
                        default:
                            break;
                        }
                    }
                }

                if (needCheckIfItIsReallyATablet &&
                    (device_data.xinput_motion == -1 ||
                     device_data.xinput_proximity_in == -1 ||
                     device_data.xinput_proximity_out == -1)) {
                    continue;
                }

                if (KisTabletDebugger::instance()->initializationDebugEnabled()) {
                    qDebug() << "###################################";
                    qDebug() << "# Adding a tablet device:" << devs->name;
                    qDebug() << "Device Type:" << KisTabletDebugger::tabletDeviceToString(deviceType);
                }

                device_data.savedAxesData.tryFetchAxesMapping(dev);

                // get the min/max value for pressure!
                any = (XAnyClassPtr) (devs->inputclassinfo);
                for (j = 0; j < devs->num_classes; j++) {
                    if (any->c_class == ValuatorClass) {
                        v = (XValuatorInfoPtr) any;
                        a = (XAxisInfoPtr) ((char *) v +
                                            sizeof (XValuatorInfo));
#if defined (Q_OS_IRIX)
#else
                        device_data.minX = a[0].min_value;
                        device_data.maxX = a[0].max_value;
                        device_data.minY = a[1].min_value;
                        device_data.maxY = a[1].max_value;
                        device_data.minPressure = a[2].min_value;
                        device_data.maxPressure = a[2].max_value;
                        device_data.minTanPressure = 0;
                        device_data.maxTanPressure = 0;
                        device_data.minZ = 0;
                        device_data.maxZ = 0;
                        device_data.minRotation = a[5].min_value;
                        device_data.maxRotation = a[5].max_value;

                        if (KisTabletDebugger::instance()->initializationDebugEnabled()) {
                            qDebug() << "# Axes limits data";
                            qDebug() << "X:       " << device_data.minX << device_data.maxX;
                            qDebug() << "Y:       " << device_data.minY << device_data.maxY;
                            qDebug() << "Z:       " << device_data.minZ << device_data.maxZ;
                            qDebug() << "Pressure:" << device_data.minPressure << device_data.maxPressure;
                            qDebug() << "Rotation:" << device_data.minRotation << device_data.maxRotation;
                            qDebug() << "T. Pres: " << device_data.minTanPressure << device_data.maxTanPressure;
                        }

#endif

                        // got the max pressure no need to go further...
                        break;
                    }
                    any = (XAnyClassPtr) ((char *) any + any->length);
                } // end of for loop

                qt_tablet_devices()->append(device_data);
            } // if (gotStylus || gotEraser)
        }
        if (KIS_X11->ptrXFreeDeviceList)
            KIS_X11->ptrXFreeDeviceList(devices);
    }
}

void fetchWacomToolId(int &deviceType, qint64 &serialId)
{
    if (ptrWacomConfigInit == 0) // we actually have the lib
        return;
    WACOMCONFIG *config = ptrWacomConfigInit(KIS_X11->display, 0);
    if (config == 0)
        return;
    WACOMDEVICE *device = ptrWacomConfigOpenDevice (config, wacomDeviceName()->constData());
    if (device == 0)
        return;
    unsigned keys[1];
    int serialInt;
    ptrWacomConfigGetRawParam (device, XWACOM_PARAM_TOOLSERIAL, &serialInt, 1, keys);
    serialId = serialInt;
    int toolId;
    ptrWacomConfigGetRawParam (device, XWACOM_PARAM_TOOLID, &toolId, 1, keys);
    switch(toolId) {
    case 0x007: /* Mouse 4D and 2D */
    case 0x017: /* Intuos3 2D Mouse */
    case 0x094:
    case 0x09c:
        deviceType = QTabletEvent::FourDMouse;
        break;
    case 0x096: /* Lens cursor */
    case 0x097: /* Intuos3 Lens cursor */
        deviceType = QTabletEvent::Puck;
        break;
    case 0x0fa:
    case 0x81b: /* Intuos3 Classic Pen Eraser */
    case 0x82a: /* Eraser */
    case 0x82b: /* Intuos3 Grip Pen Eraser */
    case 0x85a:
    case 0x91a:
    case 0x91b: /* Intuos3 Airbrush Eraser */
    case 0xd1a:
        deviceType = QTabletEvent::XFreeEraser;
        break;
    case 0x112:
    case 0x912:
    case 0x913: /* Intuos3 Airbrush */
    case 0xd12:
        deviceType = QTabletEvent::Airbrush;
        break;
    case 0x012:
    case 0x022:
    case 0x032:
    case 0x801: /* Intuos3 Inking pen */
    case 0x812: /* Inking pen */
    case 0x813: /* Intuos3 Classic Pen */
    case 0x822: /* Pen */
    case 0x823: /* Intuos3 Grip Pen */
    case 0x832: /* Stroke pen */
    case 0x842:
    case 0x852:
    case 0x885: /* Intuos3 Marker Pen */
    default: /* Unknown tool */
        deviceType = QTabletEvent::Stylus;
    }

    /* Close device and return */
    ptrWacomConfigCloseDevice (device);
    ptrWacomConfigTerm(config);
}

static Qt::MouseButtons translateMouseButtons(int s)
{
    Qt::MouseButtons ret = 0;
    if (s & Button1Mask)
        ret |= Qt::LeftButton;
    if (s & Button2Mask)
        ret |= Qt::MidButton;
    if (s & Button3Mask)
        ret |= Qt::RightButton;
    return ret;
}

static Qt::MouseButton translateMouseButton(int b)
{
    return b == Button1 ? Qt::LeftButton :
        b == Button2 ? Qt::MidButton :
        b == Button3 ? Qt::RightButton :
        Qt::LeftButton /* fallback */;
}

bool translateXinputEvent(const XEvent *ev, QTabletDeviceData *tablet, QWidget *defaultWidget)
{
    Q_ASSERT(defaultWidget);

#if defined (Q_OS_IRIX)
#endif

    Q_ASSERT(tablet != 0);

    QWidget *w = defaultWidget;
    QPoint global,
        curr;
    QPointF hiRes;
    qreal pressure = 0;
    int xTilt = 0,
        yTilt = 0,
        z = 0;
    qreal tangentialPressure = 0;
    qreal rotation = 0;
    int deviceType = QTabletEvent::NoDevice;
    int pointerType = QTabletEvent::UnknownPointer;
    const XDeviceMotionEvent *motion = 0;
    XDeviceButtonEvent *button = 0;
    KisTabletEvent::ExtraEventType t = KisTabletEvent::TabletMoveEx;
    Qt::KeyboardModifiers modifiers = 0;

#if QT_VERSION >= 0x040800
    modifiers = QApplication::queryKeyboardModifiers();
#else
    modifiers = QApplication::keyboardModifiers();
#endif

#if !defined (Q_OS_IRIX)
    XID device_id = 0;
#endif

    if (ev->type == tablet->xinput_motion) {
        motion = reinterpret_cast<const XDeviceMotionEvent*>(ev);
        t = KisTabletEvent::TabletMoveEx;
        global = QPoint(motion->x_root, motion->y_root);
        curr = QPoint(motion->x, motion->y);
#if !defined (Q_OS_IRIX)
        device_id = motion->deviceid;
#endif
    } else if (ev->type == tablet->xinput_button_press || ev->type == tablet->xinput_button_release) {
        if (ev->type == tablet->xinput_button_press) {
            t = KisTabletEvent::TabletPressEx;
        } else {
            t = KisTabletEvent::TabletReleaseEx;
        }
        button = (XDeviceButtonEvent*)ev;
        global = QPoint(button->x_root, button->y_root);
        curr = QPoint(button->x, button->y);
#if !defined (Q_OS_IRIX)
        device_id = button->deviceid;
#endif
    } else {
        qFatal("Unknown event type! Probably, 'proximity', "
               "but we don't handle it here, so this is a bug");
    }

    qint64 uid = 0;
#if defined (Q_OS_IRIX)
#else
    // We've been passed in data for a tablet device that handles this type
    // of event, but it isn't necessarily the tablet device that originated
    // the event.  Use the device id to find the originating device if we
    // have it.
    QTabletDeviceDataList *tablet_list = qt_tablet_devices();
    for (int i = 0; i < tablet_list->size(); ++i) {
        QTabletDeviceData &tab = tablet_list->operator[](i);
        if (device_id == static_cast<XDevice *>(tab.device)->device_id) {
            // Replace the tablet passed in with this one.
            tablet = &tab;
            deviceType = tab.deviceType;
            if (tab.deviceType == QTabletEvent::XFreeEraser) {
                deviceType = QTabletEvent::Stylus;
                pointerType = QTabletEvent::Eraser;
            } else if (tab.deviceType == QTabletEvent::Stylus) {
                pointerType = QTabletEvent::Pen;
            }
            break;
        }
    }

    /**
     * Touch events from Wacom tablets should not be sent as real
     * tablet events
     */
    if (tablet->isTouchWacomTablet) return false;

    fetchWacomToolId(deviceType, uid);

    QRect screenArea = qApp->desktop()->rect();

    /**
     * Some 'nice' tablet drivers (evdev) do not return the value
     * of all the axes each time. Instead they tell about the
     * recenty changed axes only, so we have to keep the state of
     * all the axes internally and update the relevant part only.
     */
    bool hasSaneData = false;
    if (motion) {
        hasSaneData =
            tablet->savedAxesData.updateAxesData(motion->first_axis,
                                                 motion->axes_count,
                                                 motion->axis_data);
    } else if (button) {
        hasSaneData =
            tablet->savedAxesData.updateAxesData(button->first_axis,
                                                 button->axes_count,
                                                 button->axis_data);
    }

    if (!hasSaneData) return false;

    hiRes = tablet->savedAxesData.position(tablet, screenArea);
    pressure = tablet->savedAxesData.pressure();
    xTilt = tablet->savedAxesData.xTilt();
    yTilt = tablet->savedAxesData.yTilt();

    rotation = std::fmod(qreal(tablet->savedAxesData.rotation() - tablet->minRotation) /
                         (tablet->maxRotation - tablet->minRotation) + 0.5, 1.0) * 360.0;

    if (deviceType == QTabletEvent::Airbrush) {
        tangentialPressure = rotation;
        rotation = 0.;
    }
#endif

    if (tablet->widgetToGetPress) {
        w = tablet->widgetToGetPress;
    } else {
        QWidget *child = w->childAt(curr);
        if (child)
            w = child;
    }
    curr = w->mapFromGlobal(global);

    if (t == KisTabletEvent::TabletPressEx) {
        tablet->widgetToGetPress = w;
    } else if (t == KisTabletEvent::TabletReleaseEx && tablet->widgetToGetPress) {
        w = tablet->widgetToGetPress;
        curr = w->mapFromGlobal(global);
        tablet->widgetToGetPress = 0;
    }

    Qt::MouseButton qtbutton = Qt::NoButton;
    Qt::MouseButtons qtbuttons;

    if (motion) {
        qtbuttons = translateMouseButtons(motion->state);
    } else if (button) {
        qtbuttons = translateMouseButtons(button->state);
        qtbutton = translateMouseButton(button->button);
    }

    KisTabletEvent e(t, curr, global, hiRes,
                     deviceType, pointerType,
                     qreal(pressure / qreal(tablet->maxPressure - tablet->minPressure)),
                     xTilt, yTilt, tangentialPressure, rotation, z, modifiers, uid,
                     qtbutton, qtbuttons);

    e.ignore();
    QApplication::sendEvent(w, &e);

    return e.isAccepted();
}

void KisTabletSupportX11::init()
{
    kis_x11_init_tablet();
}

void evdevEventsActivationWorkaround(WId window)
{
    /**
     * Evdev devices send us events *only* in case we requested
     * them for every window which desires to get them, so just
     * do it as it wants
     */
    static QSet<WId> registeredWindows;
    if (registeredWindows.contains(window)) return;

    registeredWindows.insert(window);

    QTabletDeviceDataList *tablets = qt_tablet_devices();
    for (int i = 0; i < tablets->size(); ++i) {
        QTabletDeviceData &tab = tablets->operator [](i);

        XSelectExtensionEvent(KIS_X11->display,
                              window,
                              tab.eventList,
                              tab.eventCount);
    }
}

bool KisTabletSupportX11::eventFilter(void *ev, long * /*unused_on_X11*/)
{
    XEvent *event = static_cast<XEvent*>(ev);

    // Eat the choked mouse event...
    if (kis_tabletChokeMouse &&
        (event->type == ButtonRelease ||
         event->type == ButtonPress ||
         event->type == MotionNotify)) {

        kis_tabletChokeMouse = false;

        // Mhom-mhom...
        return true;
    }

    if (kis_haveEvdevTablets && event->type == EnterNotify) {
        evdevEventsActivationWorkaround((WId)event->xany.window);
    }

    QTabletDeviceDataList *tablets = qt_tablet_devices();
    for (int i = 0; i < tablets->size(); ++i) {
        QTabletDeviceData &tab = tablets->operator [](i);
        if (event->type == tab.xinput_motion
            || event->type == tab.xinput_button_release
            || event->type == tab.xinput_button_press) {

            QWidget *widget = QApplication::activePopupWidget();

            if (!widget) {
                widget = QApplication::activeModalWidget();
            }

            if (!widget) {
                widget = QWidget::find((WId)event->xany.window);
            }

            bool retval = widget ? translateXinputEvent(event, &tab, widget) : false;

            if (retval) {
                /**
                 * If the tablet event is accepted, no mouse event
                 * should arrive. Otherwise, the popup widgets (at
                 * least) will not work correctly
                 */
                kis_tabletChokeMouse = true;
            }

            return retval;
        } else if (event->type == tab.xinput_proximity_in ||
                   event->type == tab.xinput_proximity_out) {

            const XProximityNotifyEvent *proximity =
                reinterpret_cast<const XProximityNotifyEvent*>(event);
            XID device_id = proximity->deviceid;

            QTabletDeviceDataList *tablet_list = qt_tablet_devices();
            for (int i = 0; i < tablet_list->size(); ++i) {
                QTabletDeviceData &tab = tablet_list->operator[](i);
                if (device_id == static_cast<XDevice *>(tab.device)->device_id &&
                    tab.isTouchWacomTablet) {

                    QWidget *widget = QApplication::activePopupWidget();

                    if (!widget) {
                        widget = QApplication::activeModalWidget();
                    }

                    if (!widget) {
                        widget = QWidget::find((WId)event->xany.window);
                    }

                    if (widget) {
                        QPoint curr(proximity->x, proximity->y);
                        QWidget *child = widget->childAt(curr);

                        if (child) {
                            widget = child;
                        }

                        QEvent::Type type = (QEvent::Type)
                            (event->type == tab.xinput_proximity_in ?
                             KisTabletEvent::TouchProximityInEx :
                             KisTabletEvent::TouchProximityOutEx);

                        QEvent e(type);
                        e.ignore();
                        QApplication::sendEvent(widget, &e);
                    }

                    return true;
                }
            }
        }
    }

    return false;
}
