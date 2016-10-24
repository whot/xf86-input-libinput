/*
 * Copyright © 2016 Red Hat, Inc.
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

#ifndef BEZIER_H
#define BEZIER_H

#include <stdlib.h>
#include <stdbool.h>

struct bezier_control_point {
	double x, y;
};

extern const struct bezier_control_point bezier_defaults[4];

/**
 * Given four control points in the range [(0.0/0.0), (1.0/1.0)]
 * construct a Bézier curve.
 *
 *    ^
 *1.0 |    c2 ______ c3
 *    |     _/
 *    |    /
 *    |c1 /
 *    |  /
 *    | /
 *    |/_________________>
 *    c0           1.0
 *
 * This function requires that c[i].x <= c[i+1].x
 *
 * The curve is mapped into a canvas size [0, bezier_sz)². For each x
 * coordiante in [0, bezier_sz), the matching y coordinate is thus
 * bezier[x].
 *
 * In other words, if you have a range [0,2048) input possible values,
 * the output is a list of 2048 points in a [0, 2048) range.
 *
 * @return true on success, false otherwise
 */
bool
cubic_bezier(const struct bezier_control_point controls[4],
	     int *bezier,
	     size_t bezier_sz);
#endif
