/*
 * Copyright © 2013 Red Hat, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of Red Hat
 * not be used in advertising or publicity pertaining to distribution
 * of the software without specific, written prior permission.  Red
 * Hat makes no representations about the suitability of this software
 * for any purpose.  It is provided "as is" without express or implied
 * warranty.
 *
 * THE AUTHORS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/epoll.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <xorg-server.h>
#include <exevents.h>
#include <xkbsrv.h>
#include <xf86Xinput.h>
#include <xserver-properties.h>
#include <libinput.h>
#include <linux/input.h>

#include <X11/Xatom.h>

#define TOUCHPAD_MAX_BUTTONS 7 /* three buttons, 4 scroll buttons */
#define TOUCHPAD_NUM_AXES 4 /* x, y, hscroll, vscroll */
#define TOUCH_MAX_SLOTS 15
#define XORG_KEYCODE_OFFSET 8

/*
   libinput does not provide axis information for absolute devices, instead
   it scales into the screen dimensions provided. So we set up the axes with
   a fixed range, let libinput scale into that range and then the server
   do the scaling it usually does.
 */
#define TOUCH_AXIS_MAX 0xffff

/*
   libinput scales wheel events by DEFAULT_AXIS_STEP_DISTANCE, which is
   currently 10.
 */
#define DEFAULT_LIBINPUT_AXIS_STEP_DISTANCE 10

struct xf86libinput_driver {
	struct libinput *libinput;
	int device_enabled_count;
};

static struct xf86libinput_driver driver_context;

struct xf86libinput {
	char *path;
	struct libinput_device *device;

	int scroll_vdist;
	int scroll_hdist;
	int scroll_vdist_remainder;
	int scroll_hdist_remainder;

	struct {
		double x;
		double y;
		double x_remainder;
		double y_remainder;
	} scale;

	ValuatorMask *valuators;
};

static int
LibinputSetProperty(DeviceIntPtr dev, Atom atom, XIPropertyValuePtr val,
                 BOOL checkonly);
static void
LibinputInitProperty(DeviceIntPtr dev);

static int
xf86libinput_on(DeviceIntPtr dev)
{
	InputInfoPtr pInfo = dev->public.devicePrivate;
	struct xf86libinput *driver_data = pInfo->private;
	struct libinput *libinput = driver_context.libinput;
	struct libinput_device *device = driver_data->device;

	device = libinput_path_add_device(libinput, driver_data->path);
	if (!device)
		return !Success;
	libinput_device_ref(device);
	libinput_device_set_user_data(device, pInfo);
	driver_data->device = device;

	pInfo->fd = libinput_get_fd(libinput);

	if (driver_context.device_enabled_count == 0) {
		/* Can't use xf86AddEnabledDevice on an epollfd */
		AddEnabledDevice(pInfo->fd);
	}

	driver_context.device_enabled_count++;
	dev->public.on = TRUE;

	return Success;
}

static int
xf86libinput_off(DeviceIntPtr dev)
{
	InputInfoPtr pInfo = dev->public.devicePrivate;
	struct xf86libinput *driver_data = pInfo->private;

	if (--driver_context.device_enabled_count == 0) {
		RemoveEnabledDevice(pInfo->fd);
	}

	pInfo->fd = -1;
	dev->public.on = FALSE;

	libinput_device_set_user_data(driver_data->device, NULL);
	libinput_path_remove_device(driver_data->device);
	libinput_device_unref(driver_data->device);
	driver_data->device = NULL;

	return Success;
}

static void
xf86libinput_ptr_ctl(DeviceIntPtr dev, PtrCtrl *ctl)
{
}


static void
init_button_map(unsigned char *btnmap, size_t size)
{
	int i;

	memset(btnmap, 0, size);
	for (i = 0; i <= TOUCHPAD_MAX_BUTTONS; i++)
		btnmap[i] = i;
}

static void
init_button_labels(Atom *labels, size_t size)
{
	memset(labels, 0, size * sizeof(Atom));
	labels[0] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_LEFT);
	labels[1] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_MIDDLE);
	labels[2] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_RIGHT);
	labels[3] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_WHEEL_UP);
	labels[4] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_WHEEL_DOWN);
	labels[5] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_HWHEEL_LEFT);
	labels[6] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_HWHEEL_RIGHT);
}

static void
init_axis_labels(Atom *labels, size_t size)
{
	memset(labels, 0, size * sizeof(Atom));
	labels[0] = XIGetKnownProperty(AXIS_LABEL_PROP_REL_X);
	labels[1] = XIGetKnownProperty(AXIS_LABEL_PROP_REL_Y);
	labels[2] = XIGetKnownProperty(AXIS_LABEL_PROP_REL_HSCROLL);
	labels[3] = XIGetKnownProperty(AXIS_LABEL_PROP_REL_VSCROLL);
}

static int
xf86libinput_init_pointer(InputInfoPtr pInfo)
{
	DeviceIntPtr dev= pInfo->dev;
	struct xf86libinput *driver_data = pInfo->private;
	int min, max, res;

	unsigned char btnmap[TOUCHPAD_MAX_BUTTONS + 1];
	Atom btnlabels[TOUCHPAD_MAX_BUTTONS];
	Atom axislabels[TOUCHPAD_NUM_AXES];

	init_button_map(btnmap, ARRAY_SIZE(btnmap));
	init_button_labels(btnlabels, ARRAY_SIZE(btnlabels));
	init_axis_labels(axislabels, ARRAY_SIZE(axislabels));

	InitPointerDeviceStruct((DevicePtr)dev, btnmap,
				TOUCHPAD_MAX_BUTTONS,
				btnlabels,
				xf86libinput_ptr_ctl,
				GetMotionHistorySize(),
				TOUCHPAD_NUM_AXES,
				axislabels);
	min = -1;
	max = -1;
	res = 0;

	xf86InitValuatorAxisStruct(dev, 0,
			           XIGetKnownProperty(AXIS_LABEL_PROP_REL_X),
				   min, max, res * 1000, 0, res * 1000, Relative);
	xf86InitValuatorAxisStruct(dev, 1,
			           XIGetKnownProperty(AXIS_LABEL_PROP_REL_Y),
				   min, max, res * 1000, 0, res * 1000, Relative);

	SetScrollValuator(dev, 2, SCROLL_TYPE_HORIZONTAL, driver_data->scroll_hdist, 0);
	SetScrollValuator(dev, 3, SCROLL_TYPE_VERTICAL, driver_data->scroll_vdist, 0);

	return Success;
}

static void
xf86libinput_kbd_ctrl(DeviceIntPtr device, KeybdCtrl *ctrl)
{
#define CAPSFLAG	1
#define NUMFLAG		2
#define SCROLLFLAG	4

    static struct { int xbit, code; } bits[] = {
        { CAPSFLAG,	LIBINPUT_LED_CAPS_LOCK },
        { NUMFLAG,	LIBINPUT_LED_NUM_LOCK },
        { SCROLLFLAG,	LIBINPUT_LED_SCROLL_LOCK },
	{ 0, 0 },
    };
    int i = 0;
    enum libinput_led leds = 0;
    InputInfoPtr pInfo = device->public.devicePrivate;
    struct xf86libinput *driver_data = pInfo->private;
    struct libinput_device *ldevice = driver_data->device;

    while (bits[i].xbit) {
	    if (ctrl->leds & bits[i].xbit)
		    leds |= bits[i].code;
	    i++;
    }

    libinput_device_led_update(ldevice, leds);
}

static void
xf86libinput_init_keyboard(InputInfoPtr pInfo)
{
	DeviceIntPtr dev= pInfo->dev;
	XkbRMLVOSet rmlvo = {0};

	rmlvo.rules = xf86SetStrOption(pInfo->options, "xkb_rules", "evdev");
	rmlvo.model = xf86SetStrOption(pInfo->options, "xkb_model", "pc104");
	rmlvo.layout = xf86SetStrOption(pInfo->options, "xkb_layout", "us");
	rmlvo.variant = xf86SetStrOption(pInfo->options, "xkb_variant", NULL);
	rmlvo.options = xf86SetStrOption(pInfo->options, "xkb_options", NULL);

	InitKeyboardDeviceStruct(dev, &rmlvo, NULL,
				 xf86libinput_kbd_ctrl);
	XkbFreeRMLVOSet(&rmlvo, FALSE);
}

static void
xf86libinput_init_touch(InputInfoPtr pInfo)
{
	DeviceIntPtr dev = pInfo->dev;
	int min, max, res;
	unsigned char btnmap[TOUCHPAD_MAX_BUTTONS + 1];
	Atom btnlabels[TOUCHPAD_MAX_BUTTONS];
	Atom axislabels[TOUCHPAD_NUM_AXES];

	init_button_map(btnmap, ARRAY_SIZE(btnmap));
	init_button_labels(btnlabels, ARRAY_SIZE(btnlabels));
	init_axis_labels(axislabels, ARRAY_SIZE(axislabels));

	InitPointerDeviceStruct((DevicePtr)dev, btnmap,
				TOUCHPAD_MAX_BUTTONS,
				btnlabels,
				xf86libinput_ptr_ctl,
				GetMotionHistorySize(),
				TOUCHPAD_NUM_AXES,
				axislabels);
	min = 0;
	max = TOUCH_AXIS_MAX;
	res = 0;

	xf86InitValuatorAxisStruct(dev, 0,
			           XIGetKnownProperty(AXIS_LABEL_PROP_ABS_X),
				   min, max, res * 1000, 0, res * 1000, Absolute);
	xf86InitValuatorAxisStruct(dev, 1,
			           XIGetKnownProperty(AXIS_LABEL_PROP_ABS_Y),
				   min, max, res * 1000, 0, res * 1000, Absolute);
	InitTouchClassDeviceStruct(dev, TOUCH_MAX_SLOTS, XIDirectTouch, 2);

}

static int
xf86libinput_init(DeviceIntPtr dev)
{
	InputInfoPtr pInfo = dev->public.devicePrivate;
	struct xf86libinput *driver_data = pInfo->private;
	struct libinput_device *device = driver_data->device;

	dev->public.on = FALSE;

	if (libinput_device_has_capability(device, LIBINPUT_DEVICE_CAP_KEYBOARD))
		xf86libinput_init_keyboard(pInfo);
	if (libinput_device_has_capability(device, LIBINPUT_DEVICE_CAP_POINTER))
		xf86libinput_init_pointer(pInfo);
	if (libinput_device_has_capability(device, LIBINPUT_DEVICE_CAP_TOUCH))
		xf86libinput_init_touch(pInfo);

	/* unref the device now, because we'll get a new ref during
	   DEVICE_ON */
	libinput_device_unref(device);

        LibinputInitProperty(dev);
        XIRegisterPropertyHandler(dev, LibinputSetProperty, NULL, NULL);

	return 0;
}

static void
xf86libinput_destroy(DeviceIntPtr dev)
{
}

static int
xf86libinput_device_control(DeviceIntPtr dev, int mode)
{
	int rc = BadValue;

	switch(mode) {
		case DEVICE_INIT:
			rc = xf86libinput_init(dev);
			break;
		case DEVICE_ON:
			rc = xf86libinput_on(dev);
			break;
		case DEVICE_OFF:
			rc = xf86libinput_off(dev);
			break;
		case DEVICE_CLOSE:
			xf86libinput_destroy(dev);
			break;
	}

	return rc;
}

static void
xf86libinput_handle_motion(InputInfoPtr pInfo, struct libinput_event_pointer *event)
{
	DeviceIntPtr dev = pInfo->dev;
	struct xf86libinput *driver_data = pInfo->private;
	ValuatorMask *mask = driver_data->valuators;
	double x, y;

	x = libinput_event_pointer_get_dx(event);
	y = libinput_event_pointer_get_dy(event);

	valuator_mask_zero(mask);
	valuator_mask_set_double(mask, 0, x);
	valuator_mask_set_double(mask, 1, y);

	xf86PostMotionEventM(dev, Relative, mask);
}

static void
xf86libinput_handle_button(InputInfoPtr pInfo, struct libinput_event_pointer *event)
{
	DeviceIntPtr dev = pInfo->dev;
	int button;
	int is_press;

	switch(libinput_event_pointer_get_button(event)) {
		case BTN_LEFT: button = 1; break;
		case BTN_MIDDLE: button = 2; break;
		case BTN_RIGHT: button = 3; break;
		default: /* no touchpad actually has those buttons */
			return;
	}
	is_press = (libinput_event_pointer_get_button_state(event) == LIBINPUT_BUTTON_STATE_PRESSED);
	xf86PostButtonEvent(dev, Relative, button, is_press, 0, 0);
}

static void
xf86libinput_handle_key(InputInfoPtr pInfo, struct libinput_event_keyboard *event)
{
	DeviceIntPtr dev = pInfo->dev;
	int is_press;
	int key = libinput_event_keyboard_get_key(event);

	key += XORG_KEYCODE_OFFSET;

	is_press = (libinput_event_keyboard_get_key_state(event) == LIBINPUT_KEY_STATE_PRESSED);
	xf86PostKeyboardEvent(dev, key, is_press);
}

static void
xf86libinput_handle_axis(InputInfoPtr pInfo, struct libinput_event_pointer *event)
{
	DeviceIntPtr dev = pInfo->dev;
	struct xf86libinput *driver_data = pInfo->private;
	ValuatorMask *mask = driver_data->valuators;
	int axis;
	double value;

	if (libinput_event_pointer_get_axis(event) ==
			LIBINPUT_POINTER_AXIS_SCROLL_VERTICAL)
		axis = 3;
	else
		axis = 2;

	value = libinput_event_pointer_get_axis_value(event) / DEFAULT_LIBINPUT_AXIS_STEP_DISTANCE;

	valuator_mask_zero(mask);
	valuator_mask_set_double(mask, axis, value);

	xf86PostMotionEventM(dev, Relative, mask);
}

static void
xf86libinput_handle_touch(InputInfoPtr pInfo,
			  struct libinput_event_touch *event,
			  enum libinput_event_type event_type)
{
	DeviceIntPtr dev = pInfo->dev;
	struct xf86libinput *driver_data = pInfo->private;
	int type;
	int slot;
	ValuatorMask *m = driver_data->valuators;
	double val;

	/* libinput doesn't give us hw touch ids which X expects, so
	   emulate them here */
	static int next_touchid;
	static int touchids[TOUCH_MAX_SLOTS] = {0};

	slot = libinput_event_touch_get_slot(event);

	switch (event_type) {
		case LIBINPUT_EVENT_TOUCH_DOWN:
			type = XI_TouchBegin;
			touchids[slot] = next_touchid++;
			break;
		case LIBINPUT_EVENT_TOUCH_UP:
			type = XI_TouchEnd;
			break;
		case LIBINPUT_EVENT_TOUCH_MOTION:
			type = XI_TouchUpdate;
			break;
		default:
			return;
	};

	valuator_mask_zero(m);

	if (event_type != LIBINPUT_EVENT_TOUCH_UP) {
		val = libinput_event_touch_get_x_transformed(event, TOUCH_AXIS_MAX);
		valuator_mask_set_double(m, 0, val);

		val = libinput_event_touch_get_y_transformed(event, TOUCH_AXIS_MAX);
		valuator_mask_set_double(m, 1, val);
	}

	xf86PostTouchEvent(dev, touchids[slot], type, 0, m);
}

static void
xf86libinput_handle_event(struct libinput_event *event)
{
	struct libinput_device *device;
	InputInfoPtr pInfo;

	device = libinput_event_get_device(event);
	pInfo = libinput_device_get_user_data(device);

	if (pInfo && !pInfo->dev->public.on)
		return;

	switch (libinput_event_get_type(event)) {
		case LIBINPUT_EVENT_NONE:
		case LIBINPUT_EVENT_DEVICE_ADDED:
		case LIBINPUT_EVENT_DEVICE_REMOVED:
			break;
		case LIBINPUT_EVENT_POINTER_MOTION_ABSOLUTE:
			/* FIXME */
			break;

		case LIBINPUT_EVENT_POINTER_MOTION:
			xf86libinput_handle_motion(pInfo,
						   libinput_event_get_pointer_event(event));
			break;
		case LIBINPUT_EVENT_POINTER_BUTTON:
			xf86libinput_handle_button(pInfo,
						   libinput_event_get_pointer_event(event));
			break;
		case LIBINPUT_EVENT_KEYBOARD_KEY:
			xf86libinput_handle_key(pInfo,
						libinput_event_get_keyboard_event(event));
			break;
		case LIBINPUT_EVENT_POINTER_AXIS:
			xf86libinput_handle_axis(pInfo,
						 libinput_event_get_pointer_event(event));
			break;
		case LIBINPUT_EVENT_TOUCH_FRAME:
			break;
		case LIBINPUT_EVENT_TOUCH_UP:
		case LIBINPUT_EVENT_TOUCH_DOWN:
		case LIBINPUT_EVENT_TOUCH_MOTION:
		case LIBINPUT_EVENT_TOUCH_CANCEL:
			xf86libinput_handle_touch(pInfo,
						  libinput_event_get_touch_event(event),
						  libinput_event_get_type(event));
			break;
	}
}


static void
xf86libinput_read_input(InputInfoPtr pInfo)
{
	struct libinput *libinput = driver_context.libinput;
	int rc;
	struct libinput_event *event;

        rc = libinput_dispatch(libinput);
	if (rc == -EAGAIN)
		return;

	if (rc < 0) {
		ErrorFSigSafe("Error reading events: %d\n", -rc);
		return;
	}

	while ((event = libinput_get_event(libinput))) {
		xf86libinput_handle_event(event);
		libinput_event_destroy(event);
	}
}

static int
open_restricted(const char *path, int flags, void *data)
{
	int fd = open(path, flags);
	return fd < 0 ? -errno : fd;
}

static void
close_restricted(int fd, void *data)
{
	close(fd);
}

const struct libinput_interface interface = {
    .open_restricted = open_restricted,
    .close_restricted = close_restricted,
};

static int xf86libinput_pre_init(InputDriverPtr drv,
				 InputInfoPtr pInfo,
				 int flags)
{
	struct xf86libinput *driver_data = NULL;
        struct libinput *libinput = NULL;
	struct libinput_device *device;
	char *path;

	pInfo->fd = -1;
	pInfo->type_name = XI_TOUCHPAD;
	pInfo->device_control = xf86libinput_device_control;
	pInfo->read_input = xf86libinput_read_input;
	pInfo->control_proc = NULL;
	pInfo->switch_mode = NULL;

	driver_data = calloc(1, sizeof(*driver_data));
	if (!driver_data)
		goto fail;

	driver_data->valuators = valuator_mask_new(2);
	if (!driver_data->valuators)
		goto fail;

	driver_data->scroll_vdist = 1;
	driver_data->scroll_hdist = 1;

	path = xf86SetStrOption(pInfo->options, "Device", NULL);
	if (!path)
		goto fail;

	if (!driver_context.libinput)
		driver_context.libinput = libinput_path_create_context(&interface, &driver_context);
	else
		libinput_ref(driver_context.libinput);

	libinput = driver_context.libinput;

	if (libinput == NULL) {
		xf86IDrvMsg(pInfo, X_ERROR, "Creating a device for %s failed\n", path);
		goto fail;
	}

	device = libinput_path_add_device(libinput, path);
	if (!device) {
		xf86IDrvMsg(pInfo, X_ERROR, "Failed to create a device for %s\n", path);
		goto fail;
	}

	/* We ref the device but remove it afterwards. The hope is that
	   between now and DEVICE_INIT/DEVICE_ON, the device doesn't change.
	  */
	libinput_device_ref(device);
	libinput_path_remove_device(device);

	pInfo->fd = -1;
	pInfo->private = driver_data;
	driver_data->path = path;
	driver_data->device = device;

	pInfo->options = xf86ReplaceIntOption(pInfo->options, "AccelerationProfile", -1);
	pInfo->options = xf86ReplaceStrOption(pInfo->options, "AccelerationScheme", "none");

	return Success;

fail:
	if (driver_data->valuators)
		valuator_mask_free(&driver_data->valuators);
	free(path);
	free(driver_data);
	return BadValue;
}

static void
xf86libinput_uninit(InputDriverPtr drv,
		    InputInfoPtr pInfo,
		    int flags)
{
	struct xf86libinput *driver_data = pInfo->private;
	if (driver_data) {
		driver_context.libinput = libinput_unref(driver_context.libinput);
		valuator_mask_free(&driver_data->valuators);
		free(driver_data->path);
		free(driver_data);
		pInfo->private = NULL;
	}
	xf86DeleteInput(pInfo, flags);
}

InputDriverRec xf86libinput_driver = {
	.driverVersion	= 1,
	.driverName	= "libinput",
	.PreInit	= xf86libinput_pre_init,
	.UnInit		= xf86libinput_uninit,
};

static XF86ModuleVersionInfo xf86libinput_version_info = {
	"libinput",
	MODULEVENDORSTRING,
	MODINFOSTRING1,
	MODINFOSTRING2,
	XORG_VERSION_CURRENT,
	PACKAGE_VERSION_MAJOR, PACKAGE_VERSION_MINOR, PACKAGE_VERSION_PATCHLEVEL,
	ABI_CLASS_XINPUT,
	ABI_XINPUT_VERSION,
	MOD_CLASS_XINPUT,
	{0, 0, 0, 0}
};

static pointer
xf86libinput_setup_proc(pointer module, pointer options, int *errmaj, int *errmin)
{
	xf86AddInputDriver(&xf86libinput_driver, module, 0);
	return module;
}

_X_EXPORT XF86ModuleData libinputModuleData = {
	.vers		= &xf86libinput_version_info,
	.setup		= &xf86libinput_setup_proc,
	.teardown	= NULL
};



/* Property support */

/* Tapping enabled/disabled: BOOL, 1 value */
#define PROP_TAP "libinput Tapping Enabled"
/* Calibration matrix: FLOAT, 9 values of a 3x3 matrix, in rows */
#define PROP_CALIBRATION "libinput Calibration Matrix"
/* Pointer accel speed: FLOAT, 1 value, 32 bit */
#define PROP_ACCEL "libinput Accel Speed"
/* Natural scrolling: BOOL, 1 value */
#define PROP_NATURAL_SCROLL "libinput Natural Scrolling Enabled"
/* Send-events mode: 32-bit int, 1 value */
#define PROP_SENDEVENTS "libinput Send Events Mode"

static Atom prop_float; /* server-defined */
static Atom prop_tap;
static Atom prop_calibration;
static Atom prop_accel;
static Atom prop_natural_scroll;
static Atom prop_sendevents;

static inline int
LibinputSetPropertyTap(DeviceIntPtr dev,
                       Atom atom,
                       XIPropertyValuePtr val,
                       BOOL checkonly,
		       struct libinput_device *device)
{
	BOOL* data;

	if (val->format != 8 || val->size != 1 || val->type != XA_INTEGER)
		return BadMatch;

	data = (BOOL*)val->data;
	if (checkonly) {
		if (*data != 0 && *data != 1)
			return BadValue;

		if (libinput_device_config_tap_get_finger_count(device) == 0)
			return BadMatch;
	} else {
		libinput_device_config_tap_set_enabled(device, *data);
	}

	return Success;
}

static inline int
LibinputSetPropertyCalibration(DeviceIntPtr dev,
                               Atom atom,
                               XIPropertyValuePtr val,
			       BOOL checkonly,
			       struct libinput_device *device)
{
	float* data;

	if (val->format != 32 || val->size != 9 || val->type != prop_float)
		return BadMatch;

	data = (float*)val->data;

	if (checkonly) {
		if (data[6] != 0 ||
		    data[7] != 0 ||
		    data[8] != 1)
			return BadValue;

		if (!libinput_device_config_calibration_has_matrix(device))
			return BadMatch;
	} else {
		libinput_device_config_calibration_set_matrix(device, data);
	}

	return Success;
}

static inline int
LibinputSetPropertyAccel(DeviceIntPtr dev,
			 Atom atom,
			 XIPropertyValuePtr val,
			 BOOL checkonly,
			 struct libinput_device *device)
{
	float* data;

	if (val->format != 32 || val->size != 1 || val->type != prop_float)
		return BadMatch;

	data = (float*)val->data;

	if (checkonly) {
		if (*data < -1 || *data > 1)
			return BadValue;

		if (libinput_device_config_accel_is_available(device) == 0)
			return BadMatch;
	} else {
		libinput_device_config_accel_set_speed(device, *data);
	}

	return Success;
}

static inline int
LibinputSetPropertyNaturalScroll(DeviceIntPtr dev,
                                 Atom atom,
                                 XIPropertyValuePtr val,
                                 BOOL checkonly,
                                 struct libinput_device *device)
{
	BOOL* data;

	if (val->format != 8 || val->size != 1 || val->type != XA_INTEGER)
		return BadMatch;

	data = (BOOL*)val->data;

	if (checkonly) {
		if (*data != 0 && *data != 1)
			return BadValue;

		if (libinput_device_config_scroll_has_natural_scroll(device) == 0)
			return BadMatch;
	} else {
		libinput_device_config_scroll_set_natural_scroll_enabled(device, *data);
	}

	return Success;
}

static inline int
LibinputSetPropertySendEvents(DeviceIntPtr dev,
			      Atom atom,
			      XIPropertyValuePtr val,
			      BOOL checkonly,
			      struct libinput_device *device)
{
	CARD32* data;

	if (val->format != 32 || val->size != 1 || val->type != XA_CARDINAL)
		return BadMatch;

	data = (CARD32*)val->data;

	if (checkonly) {
		uint32_t supported = libinput_device_config_send_events_get_modes(device);
		uint32_t new_mode = *data;

		if ((new_mode | supported) != supported)
			return BadValue;

		/* Only one bit must be set */
		if (!new_mode || ((new_mode & (new_mode - 1)) != 0))
			return BadValue;
	} else {
		libinput_device_config_send_events_set_mode(device, *data);
	}

	return Success;
}

static int
LibinputSetProperty(DeviceIntPtr dev, Atom atom, XIPropertyValuePtr val,
                 BOOL checkonly)
{
	InputInfoPtr pInfo  = dev->public.devicePrivate;
	struct xf86libinput *driver_data = pInfo->private;
	struct libinput_device *device = driver_data->device;

	if (atom == prop_tap)
		return LibinputSetPropertyTap(dev, atom, val, checkonly, device);
	else if (atom == prop_calibration)
		return LibinputSetPropertyCalibration(dev, atom, val,
						      checkonly, device);
	else if (atom == prop_accel)
		return LibinputSetPropertyAccel(dev, atom, val,
						checkonly, device);
	else if (atom == prop_natural_scroll)
		return LibinputSetPropertyNaturalScroll(dev, atom, val,
							checkonly, device);
	else if (atom == prop_sendevents)
		return LibinputSetPropertySendEvents(dev, atom, val,
						     checkonly, device);

	return Success;
}

static void
LibinputInitProperty(DeviceIntPtr dev)
{
	InputInfoPtr pInfo  = dev->public.devicePrivate;
	struct xf86libinput *driver_data = pInfo->private;
	struct libinput_device *device = driver_data->device;
	int rc;

	prop_float = XIGetKnownProperty("FLOAT");

	if (libinput_device_config_tap_get_finger_count(device) > 0) {
		BOOL tap = libinput_device_config_tap_get_enabled(device);

		prop_tap = MakeAtom(PROP_TAP, strlen(PROP_TAP), TRUE);
		rc = XIChangeDeviceProperty(dev, prop_tap, XA_INTEGER, 8,
					    PropModeReplace, 1, &tap, FALSE);
		if (rc != Success)
			return;
		XISetDevicePropertyDeletable(dev, prop_tap, FALSE);
	}

	/* We use a 9-element matrix just to be closer to the X server's
	   transformation matrix which also has the full matrix */
	if (libinput_device_config_calibration_has_matrix(device)) {
		float calibration[9];

		libinput_device_config_calibration_get_matrix(device, calibration);
		calibration[6] = 0;
		calibration[7] = 0;
		calibration[8] = 1;

		prop_calibration = MakeAtom(PROP_CALIBRATION,
					    strlen(PROP_CALIBRATION),
					    TRUE);

		rc = XIChangeDeviceProperty(dev, prop_calibration, prop_float, 32,
					    PropModeReplace, 9, calibration, FALSE);
		if (rc != Success)
			return;
		XISetDevicePropertyDeletable(dev, prop_calibration, FALSE);
	}

	if (libinput_device_config_accel_is_available(device)) {
		float speed = libinput_device_config_accel_get_speed(device);

		prop_accel = MakeAtom(PROP_ACCEL, strlen(PROP_ACCEL), TRUE);
		rc = XIChangeDeviceProperty(dev, prop_accel, prop_float, 32,
					    PropModeReplace, 1, &speed, FALSE);
		if (rc != Success)
			return;
		XISetDevicePropertyDeletable(dev, prop_accel, FALSE);
	}

	if (libinput_device_config_scroll_has_natural_scroll(device)) {
		BOOL natural_scroll = libinput_device_config_scroll_get_natural_scroll_enabled(device);

		prop_natural_scroll = MakeAtom(PROP_NATURAL_SCROLL,
					       strlen(PROP_NATURAL_SCROLL),
					       TRUE);
		rc = XIChangeDeviceProperty(dev, prop_natural_scroll, XA_INTEGER, 8,
					    PropModeReplace, 1, &natural_scroll, FALSE);
		if (rc != Success)
			return;
		XISetDevicePropertyDeletable(dev, prop_natural_scroll, FALSE);
	}

	if (libinput_device_config_send_events_get_modes(device) !=
		    LIBINPUT_CONFIG_SEND_EVENTS_ENABLED) {
		uint32_t sendevents = libinput_device_config_send_events_get_mode(device);

		prop_sendevents = MakeAtom(PROP_SENDEVENTS,
					   strlen(PROP_SENDEVENTS),
					   TRUE);
		rc = XIChangeDeviceProperty(dev, prop_sendevents,
					    XA_CARDINAL, 32,
					    PropModeReplace, 1, &sendevents, FALSE);
		if (rc != Success)
			return;
		XISetDevicePropertyDeletable(dev, prop_sendevents, FALSE);

	}
}
