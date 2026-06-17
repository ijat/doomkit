/* =============================================================================
 *  doomgeneric_capi.c  --  Implementation of the flat C ABI shim.
 * -----------------------------------------------------------------------------
 *  GNU General Public License v2. See LICENSE.
 * =============================================================================
 *
 *  This file provides the six DG_* symbols the engine links against, and makes
 *  each one forward to a function pointer the host registered via
 *  dg_set_callbacks(). It is the bridge between the engine's "define these
 *  symbols" contract and an FFI's "register these function pointers" world.
 *
 *  Compile it together with the DOOM engine sources, EXCLUDING any
 *  doomgeneric_*.c platform file (those define DG_* too and would collide).
 *  It needs the engine's headers on the include path:
 *      cc -fPIC -I<engine_dir> -c bindings/doomgeneric_capi.c
 * ===========================================================================*/

#include "doomgeneric_capi.h"

/* The engine's own header: declares DG_ScreenBuffer, DOOMGENERIC_RESX/RESY and
 * doomgeneric_Create()/doomgeneric_Tick(). Provided by the upstream engine. */
#include "doomgeneric.h"

/* The single set of callbacks registered by the host. Zero-initialised, so all
 * pointers start NULL and the engine simply gets no-ops until registration. */
static dg_callbacks g_cb;

void dg_set_callbacks(const dg_callbacks *callbacks)
{
    if (callbacks) {
        g_cb = *callbacks;   /* copy by value; struct need not outlive the call */
    }
}

/* ---- The DG_* symbols the engine calls. Each forwards to g_cb. ------------- *
 *  Every pointer is null-checked so a host that omits an optional callback (or
 *  registers nothing at all) never crashes the engine.
 * -------------------------------------------------------------------------- */

void DG_Init(void)
{
    if (g_cb.init) {
        g_cb.init();
    }
}

void DG_DrawFrame(void)
{
    if (g_cb.draw_frame) {
        g_cb.draw_frame();
    }
}

void DG_SleepMs(uint32_t ms)
{
    if (g_cb.sleep_ms) {
        g_cb.sleep_ms(ms);
    }
}

uint32_t DG_GetTicksMs(void)
{
    return g_cb.get_ticks_ms ? g_cb.get_ticks_ms() : 0;
}

int DG_GetKey(int *pressed, unsigned char *doom_key)
{
    return g_cb.get_key ? g_cb.get_key(pressed, doom_key) : 0;
}

void DG_SetWindowTitle(const char *title)
{
    if (g_cb.set_window_title) {
        g_cb.set_window_title(title);
    }
}

/* ---- Lifecycle + accessors: thin, stable wrappers over the engine. -------- */

void dg_create(int argc, char **argv)
{
    doomgeneric_Create(argc, argv);
}

void dg_tick(void)
{
    doomgeneric_Tick();
}

uint32_t *dg_screen_buffer(void)
{
    /* DG_ScreenBuffer is pixel_t* (uint32_t* in the default 32-bit build). */
    return (uint32_t *)DG_ScreenBuffer;
}

int dg_resx(void)
{
    return DOOMGENERIC_RESX;
}

int dg_resy(void)
{
    return DOOMGENERIC_RESY;
}
