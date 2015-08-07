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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef DRAGLOCK_H
#define DRAGLOCK_H 1

#include <stdbool.h>
#include <stdlib.h>

/* 32 buttons are enough for everybody™
 * Note that this is the limit of physical buttons as well as the highest
 * allowed target button.
 */
#define DRAGLOCK_MAX_BUTTONS 32

enum draglock_mode
{
	DRAGLOCK_DISABLED,
	DRAGLOCK_META,
	DRAGLOCK_PAIRS
};

enum draglock_button_state
{
	DRAGLOCK_BUTTON_STATE_NONE,
	DRAGLOCK_BUTTON_STATE_DOWN_1,
	DRAGLOCK_BUTTON_STATE_UP_1,
	DRAGLOCK_BUTTON_STATE_DOWN_2,
};

struct draglock
{
	enum draglock_mode mode;
	int meta_button;			/* meta key to lock any button */
	bool meta_state;			/* meta_button state */
	unsigned int lock_pair[DRAGLOCK_MAX_BUTTONS + 1];/* specify a meta/lock pair */
	enum draglock_button_state lock_state[DRAGLOCK_MAX_BUTTONS + 1];	/* state of any locked buttons */
};

/**
 * Initialize the draglock struct based on the config string. The string is
 * either a single number to configure DRAGLOCK_META mode or a list of
 * number pairs, with pair[0] as button and pair[1] as target lock number to
 * configure DRAGLOCK_PAIRS mode.
 *
 * If config is NULL, the empty string, "0" or an even-numbered list of 0,
 * the drag lock mode is DRAGLOCK_DISABLED.
 *
 * @return 0 on success or nonzero on error
 */
int
draglock_init_from_string(struct draglock *dl, const char *config);

/**
 * Get the current drag lock mode.
 *
 * If the mode is DRAGLOCK_META, a meta button click will cause the next
 * subsequent button click to be held logically down until the release of
 * the second button click of that same button. Events from the meta button
 * are always discarded.
 *
 * If the mode is DRAGLOCK_PAIRS, any button may be configured with a
 * 'target' button number. A click of that button causes the target button
 * to be held logically down until the release of the second button click.
 */
enum draglock_mode
draglock_get_mode(const struct draglock *dl);

/**
 * @return the meta button number or 0 if the current mode is not
 * DRAGLOCK_META.
 */
int
draglock_get_meta(const struct draglock *dl);

/**
 * Get the drag lock button mapping pairs. The array is filled with the
 * button number as index and the mapped target button number as value, i.e.
 * array[3] == 8 means button 3 will draglock button 8.
 *
 * A value of 0 indicates draglock is disabled for that button.
 *
 * @note Button numbers start at 1, array[0] is always 0.
 *
 * @param[in|out] array Caller-allocated array to hold the button mappings.
 * @param[in] sz Maximum number of elements in array
 *
 * @return The number of valid elements in array or 0 if the current mode is
 * not DRAGLOCK_PAIRS
 */
size_t
draglock_get_pairs(const struct draglock *dl, int *array, size_t sz);

/**
 * Set the drag lock config to the DRAGLOCK_META mode, with the given
 * button as meta button.
 *
 * If the button is 0 the mode becomes DRAGLOCK_DISABLED.
 *
 * @return 0 on success, nonzero otherwise
 */
int
draglock_set_meta(struct draglock *dl, int meta_button);

/**
 * Set the drag lock config to the DRAGLOCK_PAIRS mode. The array
 * must be filled with the button number as index and the mapped target
 * button number as value, i.e.
 * array[3] == 8 means button 3 will draglock button 8.
 *
 * A value of 0 indicates draglock is disabled for that button. If all
 * buttons are 0, the mode becomes DRAGLOCK_DISABLED.
 *
 * @note Button numbers start at 1, array[0] is always 0.
 *
 * @return 0 on successor nonzero otherwise
 */
int
draglock_set_pairs(struct draglock *dl, const int *array, size_t sz);

/**
 * Process the given button event through the drag lock state machine.
 * If the event is to be discarded by the caller, button is set to 0.
 * Otherwise, button is set to the button event to process and is_press is
 * set to the button state to process.
 *
 * @param[in|out] button The button number to process
 * @param[in|out] is_press nonzero for press, zero for release
 *
 * @return 0 on success or 1 on error
 */
int
draglock_filter_button(struct draglock *dl, int *button, int *is_press);

#endif /* DRAGLOCK_H */
