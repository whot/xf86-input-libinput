/*
 * Copyright © 2015 Red Hat, Inc.
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

#ifndef _LIBINPUT_PROPERTIES_H_
#define _LIBINPUT_PROPERTIES_H_

/* Tapping enabled/disabled: BOOL, 1 value */
#define LIBINPUT_PROP_TAP "libinput Tapping Enabled"

/* Calibration matrix: FLOAT, 9 values of a 3x3 matrix, in rows */
#define LIBINPUT_PROP_CALIBRATION "libinput Calibration Matrix"

/* Pointer accel speed: FLOAT, 1 value, 32 bit */
#define LIBINPUT_PROP_ACCEL "libinput Accel Speed"

/* Natural scrolling: BOOL, 1 value */
#define LIBINPUT_PROP_NATURAL_SCROLL "libinput Natural Scrolling Enabled"

/* Send-events mode: BOOL read-only, 2 values in order disabled,
   disabled-on-external-mouse */
#define LIBINPUT_PROP_SENDEVENTS_AVAILABLE "libinput Send Events Modes Available"

/* Send-events mode: BOOL, 2 values in order disabled,
   disabled-on-external-mouse */
#define LIBINPUT_PROP_SENDEVENTS_ENABLED "libinput Send Events Mode Enabled"

/* Left-handed enabled/disabled: BOOL, 1 value */
#define LIBINPUT_PROP_LEFT_HANDED "libinput Left Handed Enabled"

/* Scroll method: BOOL read-only, 3 values in order 2fg, edge, button.
   shows available scroll methods */
#define LIBINPUT_PROP_SCROLL_METHODS_AVAILABLE "libinput Scroll Methods Available"

/* Scroll method: BOOL, 3 values in order 2fg, edge, button
   only one is enabled at a time at max */
#define LIBINPUT_PROP_SCROLL_METHOD_ENABLED "libinput Scroll Method Enabled"

/* Scroll button for button scrolling: 32-bit int, 1 value */
#define LIBINPUT_PROP_SCROLL_BUTTON "libinput Button Scrolling Button"

#endif /* _LIBINPUT_PROPERTIES_H_ */
