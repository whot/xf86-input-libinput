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

/* Tapping default enabled/disabled: BOOL, 1 value, read-only */
#define LIBINPUT_PROP_TAP_DEFAULT "libinput Tapping Enabled Default"

/* Tap drag enabled/disabled: BOOL, 1 value */
#define LIBINPUT_PROP_TAP_DRAG "libinput Tapping Drag Enabled"

/* Tap drag default enabled/disabled: BOOL, 1 value, read-only */
#define LIBINPUT_PROP_TAP_DRAG_DEFAULT "libinput Tapping Drag Enabled Default"

/* Tap drag lock enabled/disabled: BOOL, 1 value */
#define LIBINPUT_PROP_TAP_DRAG_LOCK "libinput Tapping Drag Lock Enabled"

/* Tap drag lock default enabled/disabled: BOOL, 1 value, read-only */
#define LIBINPUT_PROP_TAP_DRAG_LOCK_DEFAULT "libinput Tapping Drag Lock Enabled Default"

/* Calibration matrix: FLOAT, 9 values of a 3x3 matrix, in rows */
#define LIBINPUT_PROP_CALIBRATION "libinput Calibration Matrix"

/* Calibration matrix: FLOAT, 9 values of a 3x3 matrix, in rows, read-only*/
#define LIBINPUT_PROP_CALIBRATION_DEFAULT "libinput Calibration Matrix Default"

/* Pointer accel speed: FLOAT, 1 value, 32 bit */
#define LIBINPUT_PROP_ACCEL "libinput Accel Speed"

/* Pointer accel speed: FLOAT, 1 value, 32 bit, read-only*/
#define LIBINPUT_PROP_ACCEL_DEFAULT "libinput Accel Speed Default"

/* Pointer accel profile: BOOL, 2 values in oder adaptive, flat,
 * only one is enabled at a time at max, read-only */
#define LIBINPUT_PROP_ACCEL_PROFILES_AVAILABLE "libinput Accel Profiles Available"

/* Pointer accel profile: BOOL, 2 values in order adaptive, flat,
   only one is enabled at a time at max, read-only */
#define LIBINPUT_PROP_ACCEL_PROFILE_ENABLED_DEFAULT "libinput Accel Profile Enabled Default"

/* Pointer accel profile: BOOL, 2 values in order adaptive, flat,
   only one is enabled at a time at max */
#define LIBINPUT_PROP_ACCEL_PROFILE_ENABLED "libinput Accel Profile Enabled"

/* Natural scrolling: BOOL, 1 value */
#define LIBINPUT_PROP_NATURAL_SCROLL "libinput Natural Scrolling Enabled"

/* Natural scrolling: BOOL, 1 value, read-only */
#define LIBINPUT_PROP_NATURAL_SCROLL_DEFAULT "libinput Natural Scrolling Enabled Default"

/* Send-events mode: BOOL read-only, 2 values in order disabled,
   disabled-on-external-mouse */
#define LIBINPUT_PROP_SENDEVENTS_AVAILABLE "libinput Send Events Modes Available"

/* Send-events mode: BOOL, 2 values in order disabled,
   disabled-on-external-mouse */
#define LIBINPUT_PROP_SENDEVENTS_ENABLED "libinput Send Events Mode Enabled"

/* Send-events mode: BOOL, 2 values in order disabled,
   disabled-on-external-mouse, read-only */
#define LIBINPUT_PROP_SENDEVENTS_ENABLED_DEFAULT "libinput Send Events Mode Enabled Default"

/* Left-handed enabled/disabled: BOOL, 1 value */
#define LIBINPUT_PROP_LEFT_HANDED "libinput Left Handed Enabled"

/* Left-handed enabled/disabled: BOOL, 1 value, read-only */
#define LIBINPUT_PROP_LEFT_HANDED_DEFAULT "libinput Left Handed Enabled Default"

/* Scroll method: BOOL read-only, 3 values in order 2fg, edge, button.
   shows available scroll methods */
#define LIBINPUT_PROP_SCROLL_METHODS_AVAILABLE "libinput Scroll Methods Available"

/* Scroll method: BOOL, 3 values in order 2fg, edge, button
   only one is enabled at a time at max */
#define LIBINPUT_PROP_SCROLL_METHOD_ENABLED "libinput Scroll Method Enabled"

/* Scroll method: BOOL, 3 values in order 2fg, edge, button
   only one is enabled at a time at max, read-only */
#define LIBINPUT_PROP_SCROLL_METHOD_ENABLED_DEFAULT "libinput Scroll Method Enabled Default"

/* Scroll button for button scrolling: 32-bit int, 1 value */
#define LIBINPUT_PROP_SCROLL_BUTTON "libinput Button Scrolling Button"

/* Scroll button for button scrolling: 32-bit int, 1 value, read-only */
#define LIBINPUT_PROP_SCROLL_BUTTON_DEFAULT "libinput Button Scrolling Button Default"

/* Click method: BOOL read-only, 2 values in order buttonareas, clickfinger
   shows available click methods */
#define LIBINPUT_PROP_CLICK_METHODS_AVAILABLE "libinput Click Methods Available"

/* Click method: BOOL, 2 values in order buttonareas, clickfinger
   only one enabled at a time at max */
#define LIBINPUT_PROP_CLICK_METHOD_ENABLED "libinput Click Method Enabled"

/* Click method: BOOL, 2 values in order buttonareas, clickfinger
   only one enabled at a time at max, read-only */
#define LIBINPUT_PROP_CLICK_METHOD_ENABLED_DEFAULT "libinput Click Method Enabled Default"

/* Middle button emulation: BOOL, 1 value */
#define LIBINPUT_PROP_MIDDLE_EMULATION_ENABLED "libinput Middle Emulation Enabled"

/* Middle button emulation: BOOL, 1 value, read-only */
#define LIBINPUT_PROP_MIDDLE_EMULATION_ENABLED_DEFAULT "libinput Middle Emulation Enabled Default"

/* Disable while typing: BOOL, 1 value */
#define LIBINPUT_PROP_DISABLE_WHILE_TYPING "libinput Disable While Typing Enabled"

/* Disable while typing: BOOL, 1 value, read-only */
#define LIBINPUT_PROP_DISABLE_WHILE_TYPING_DEFAULT "libinput Disable While Typing Enabled Default"

/* Drag lock buttons, either:
   CARD8, one value, the meta lock button, or
   CARD8, n * 2 values, the drag lock pairs with n being the button and n+1
   the target button number */
#define LIBINPUT_PROP_DRAG_LOCK_BUTTONS "libinput Drag Lock Buttons"

/* Horizontal scroll events enabled: BOOL, 1 value (0 or 1).
 * If disabled, horizontal scroll events are discarded */
#define LIBINPUT_PROP_HORIZ_SCROLL_ENABLED "libinput Horizontal Scroll Enabled"

#endif /* _LIBINPUT_PROPERTIES_H_ */
