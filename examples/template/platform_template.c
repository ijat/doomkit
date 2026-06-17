/* =============================================================================
 *  platform_template.c  --  Fill-in-the-blanks skeleton for a NEW port.
 * -----------------------------------------------------------------------------
 *  GNU General Public License v2. See LICENSE.
 * =============================================================================
 *
 *  Copy this file to platform_<yourthing>.c and fill in the SIX TODOs. That is
 *  the entire job of porting DOOM with genericdoom. Each callback's contract is
 *  documented in include/genericdoom/genericdoom.h and docs/CONTRACT.md.
 *
 *  The helper modules below are optional but handle the two fiddly parts for
 *  you (key translation and a thread-safe-ish event queue). Delete the includes
 *  if your platform does not need them.
 * ===========================================================================*/

#include "genericdoom/genericdoom.h"
#include "genericdoom/dg_keyqueue.h"   /* a ready-made input ring buffer      */
#include "genericdoom/dg_keymap.h"     /* SDL-style key -> DOOM key (adaptable) */

/* A queue to bridge "OS delivers a key" and "engine asks for a key". */
static dg_keyqueue_t s_keys;

/* ---------------------------------------------------------------------------
 *  1. DG_Init -- open your display and prepare input.
 * ------------------------------------------------------------------------- */
void DG_Init(void)
{
    dg_keyqueue_init(&s_keys);

    /* TODO: create a window / framebuffer of size
     *       DOOMGENERIC_RESX x DOOMGENERIC_RESY, and set up keyboard input. */
}

/* ---------------------------------------------------------------------------
 *  2. DG_DrawFrame -- present DG_ScreenBuffer, then pump input.
 * ------------------------------------------------------------------------- */
void DG_DrawFrame(void)
{
    /* TODO: copy DG_ScreenBuffer (DOOMGENERIC_RESX*DOOMGENERIC_RESY pixels,
     *       0x00RRGGBB each) to your screen. */

    /* TODO: for each key event your OS reports this frame, do:
     *
     *   unsigned char dk = dg_keymap_from_sdl(host_key);  // or your own map
     *   dg_keyqueue_push(&s_keys, is_down ? 1 : 0, dk);
     */
}

/* ---------------------------------------------------------------------------
 *  3. DG_SleepMs -- block for `ms` milliseconds.
 * ------------------------------------------------------------------------- */
void DG_SleepMs(uint32_t ms)
{
    (void)ms;
    /* TODO: sleep ms milliseconds (e.g. usleep(ms*1000) or your RTOS delay). */
}

/* ---------------------------------------------------------------------------
 *  4. DG_GetTicksMs -- monotonic milliseconds since program start.
 * ------------------------------------------------------------------------- */
uint32_t DG_GetTicksMs(void)
{
    /* TODO: return a millisecond counter that only ever goes up. */
    return 0;
}

/* ---------------------------------------------------------------------------
 *  5. DG_GetKey -- hand the engine one queued key event, or 0 if none.
 *     Using dg_keyqueue, this is a one-liner.
 * ------------------------------------------------------------------------- */
int DG_GetKey(int *pressed, unsigned char *doomKey)
{
    return dg_keyqueue_pop(&s_keys, pressed, doomKey);
}

/* ---------------------------------------------------------------------------
 *  6. DG_SetWindowTitle -- optional; leave empty if not applicable.
 * ------------------------------------------------------------------------- */
void DG_SetWindowTitle(const char *title)
{
    (void)title;
    /* TODO (optional): set your window's title. */
}

/* ---------------------------------------------------------------------------
 *  main -- start the engine, then tick it forever.
 *  (Some platforms drive the loop differently; see examples/minimal_main.c.)
 * ------------------------------------------------------------------------- */
int main(int argc, char **argv)
{
    doomgeneric_Create(argc, argv);
    for (;;) {
        doomgeneric_Tick();
    }
    return 0;
}
