/* =============================================================================
 *  dg_keyqueue.c  --  Fixed-size circular buffer of keyboard events.
 *  See include/genericdoom/dg_keyqueue.h for the full explanation.
 *  GNU General Public License v2. See LICENSE.
 * ===========================================================================*/

#include "genericdoom/dg_keyqueue.h"

/*
 *  A note on the index arithmetic
 *  ------------------------------
 *  Both indices advance forward and wrap around using "% DG_KEYQUEUE_SIZE".
 *  The queue is empty when the two indices are equal, and full when advancing
 *  the write index by one would land on the read index. That "leave one slot
 *  empty" trick is what lets a single comparison distinguish empty from full.
 */

void dg_keyqueue_init(dg_keyqueue_t *q)
{
    /* We only need to reset the indices; stale bytes in `buffer` are harmless
     * because they can never be read while the queue reports itself empty. */
    q->write_index = 0;
    q->read_index = 0;
}

int dg_keyqueue_is_empty(const dg_keyqueue_t *q)
{
    return q->write_index == q->read_index;
}

int dg_keyqueue_is_full(const dg_keyqueue_t *q)
{
    /* Full means: the slot just before the read index is the one we'd write. */
    unsigned int next = (q->write_index + 1) % DG_KEYQUEUE_SIZE;
    return next == q->read_index;
}

unsigned int dg_keyqueue_count(const dg_keyqueue_t *q)
{
    /* Forward distance from read to write, wrapped into [0, SIZE). */
    return (q->write_index + DG_KEYQUEUE_SIZE - q->read_index) % DG_KEYQUEUE_SIZE;
}

int dg_keyqueue_push(dg_keyqueue_t *q, int pressed, unsigned char doomKey)
{
    if (dg_keyqueue_is_full(q)) {
        /* Drop the event rather than clobber unread input. */
        return 0;
    }

    /* Normalise `pressed` to exactly 1 or 0, then pack into one 16-bit word:
     * high byte = pressed flag, low byte = DOOM key code. */
    uint16_t packed = (uint16_t)(((pressed ? 1u : 0u) << 8) | doomKey);

    q->buffer[q->write_index] = packed;
    q->write_index = (q->write_index + 1) % DG_KEYQUEUE_SIZE;
    return 1;
}

int dg_keyqueue_pop(dg_keyqueue_t *q, int *pressed, unsigned char *doomKey)
{
    if (dg_keyqueue_is_empty(q)) {
        return 0;
    }

    uint16_t packed = q->buffer[q->read_index];
    q->read_index = (q->read_index + 1) % DG_KEYQUEUE_SIZE;

    /* Unpack back into the two out-parameters. */
    *pressed = (packed >> 8) & 0x01;
    *doomKey = (unsigned char)(packed & 0xFF);
    return 1;
}
