/* =============================================================================
 *  dg_keyqueue.h  --  A tiny lock-free-ish ring buffer for keyboard events.
 * -----------------------------------------------------------------------------
 *  Derived from the key queue in doomgeneric's platform ports.
 *  GNU General Public License v2. See LICENSE.
 * =============================================================================
 *
 *  THE PROBLEM THIS SOLVES
 *  -----------------------
 *  Your operating system delivers key presses whenever it feels like it
 *  (usually while you pump the event queue inside DG_DrawFrame). The DOOM
 *  engine, on the other hand, asks for keys on its own schedule via
 *  DG_GetKey(). You need somewhere to park events in between -- a small queue
 *  that one side writes to and the other side reads from.
 *
 *  This is a classic fixed-size circular (ring) buffer. It never allocates
 *  memory, so it is safe to use on the most constrained platforms.
 *
 *  HOW EVENTS ARE PACKED
 *  ---------------------
 *  Each event is squeezed into a single 16-bit word:
 *
 *      bit 15 .......... bit 8 | bit 7 .......... bit 0
 *      +----------------------+----------------------+
 *      |   pressed (1 or 0)   |   DOOM key code      |
 *      +----------------------+----------------------+
 *
 *  i.e. (pressed << 8) | doomKey. dg_keyqueue_pop() unpacks it back out for you.
 *
 *  OVERFLOW BEHAVIOUR (read this!)
 *  ------------------------------
 *  The queue holds DG_KEYQUEUE_SIZE - 1 events. This is standard for a ring
 *  buffer: one slot is always left empty so that "write index == read index"
 *  unambiguously means "empty" rather than "full". If you push into a full
 *  queue, the push is dropped (the oldest events are preserved). Sixteen slots
 *  is far more than a single frame ever needs, so in practice it never fills.
 * ===========================================================================*/

#ifndef DOOMKIT_DG_KEYQUEUE_H
#define DOOMKIT_DG_KEYQUEUE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Number of slots in the ring. Must be a power of two is NOT required, but the
 * usable capacity is always one less than this (see overflow note above). */
#ifndef DG_KEYQUEUE_SIZE
#define DG_KEYQUEUE_SIZE 16
#endif

/*
 *  The queue state. Treat the fields as private -- always go through the
 *  functions below. Declare one of these per input source (you almost always
 *  need exactly one).
 */
typedef struct {
    uint16_t buffer[DG_KEYQUEUE_SIZE]; /* packed events */
    unsigned int write_index;          /* next slot to write */
    unsigned int read_index;           /* next slot to read  */
} dg_keyqueue_t;

/*
 *  Reset a queue to empty. Call once before first use. (Zero-initialising the
 *  struct, e.g. with `= {0}`, has the same effect, but this is explicit.)
 */
void dg_keyqueue_init(dg_keyqueue_t *q);

/*
 *  True if the queue currently holds no events.
 */
int dg_keyqueue_is_empty(const dg_keyqueue_t *q);

/*
 *  True if the queue is full and the next push would be dropped.
 */
int dg_keyqueue_is_full(const dg_keyqueue_t *q);

/*
 *  How many events are waiting to be read.
 */
unsigned int dg_keyqueue_count(const dg_keyqueue_t *q);

/*
 *  Push one key event.
 *    pressed  -- non-zero for key-down, zero for key-up (stored as exactly 0/1)
 *    doomKey  -- a DOOM key code from dg_keys.h
 *  Returns 1 if the event was stored, or 0 if the queue was full (dropped).
 */
int dg_keyqueue_push(dg_keyqueue_t *q, int pressed, unsigned char doomKey);

/*
 *  Pop the oldest event in FIFO order.
 *    On success writes *pressed (0/1) and *doomKey and returns 1.
 *    If the queue is empty, writes nothing and returns 0.
 *  This mirrors the contract of DG_GetKey() exactly, so a port's DG_GetKey can
 *  be a one-line wrapper around this.
 */
int dg_keyqueue_pop(dg_keyqueue_t *q, int *pressed, unsigned char *doomKey);

#ifdef __cplusplus
}
#endif

#endif /* DOOMKIT_DG_KEYQUEUE_H */
