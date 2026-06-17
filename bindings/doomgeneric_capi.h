/* =============================================================================
 *  doomgeneric_capi.h  --  A flat C ABI so ANY language can drive the engine.
 * -----------------------------------------------------------------------------
 *  GNU General Public License v2. See LICENSE.
 * =============================================================================
 *
 *  WHY THIS LAYER EXISTS
 *  ---------------------
 *  The plain doomkit contract expects the platform to *define* six symbols
 *  (DG_Init, DG_DrawFrame, ...). That is perfect when you compile your platform
 *  code together with the engine in one C/C++ build. But it does NOT work when
 *  you want to drive the engine from Go, C#, Java, Kotlin, Python, Rust, ...
 *  because those languages cannot define raw C link-time symbols.
 *
 *  So this shim inverts the wiring into something every foreign-function
 *  interface (FFI) understands: instead of you *defining* the callbacks, you
 *  *register* them as function pointers at runtime. The shim itself provides the
 *  real DG_* symbols the engine links against, and each one simply forwards to
 *  the pointer you registered.
 *
 *      engine  --calls-->  DG_DrawFrame (in this shim)  --calls-->  your callback
 *
 *  Build this file together with the DOOM engine sources (minus any
 *  doomgeneric_*.c platform file) into a shared library:
 *
 *      libdoomgeneric.so   (Linux)
 *      libdoomgeneric.dylib(macOS)
 *      doomgeneric.dll     (Windows)
 *
 *  Then load that library from any language and call the functions below. See
 *  bindings/README.md and examples/languages/ for one worked example per
 *  language.
 *
 *  EVERYTHING HERE IS PLAIN C, extern "C", with a stable, minimal ABI:
 *    - only primitive types and pointers cross the boundary,
 *    - the callbacks use the cdecl calling convention,
 *    - the framebuffer is exposed as a raw pointer you read each frame.
 * ===========================================================================*/

#ifndef DOOMGENERIC_CAPI_H
#define DOOMGENERIC_CAPI_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Mark the public symbols for export on every platform. */
#if defined(_WIN32)
#  define DG_API __declspec(dllexport)
#else
#  define DG_API __attribute__((visibility("default")))
#endif

/* ---- The six callback signatures, as function-pointer types --------------- *
 *  These mirror the doomkit contract exactly (see doomkit.h):
 *    init             -- open display + input
 *    draw_frame       -- present the framebuffer, pump OS events
 *    sleep_ms         -- block for ms milliseconds
 *    get_ticks_ms     -- monotonic milliseconds since start
 *    get_key          -- write *pressed (0/1) and *doom_key, return 1; or 0
 *    set_window_title -- optional; may be left NULL
 * -------------------------------------------------------------------------- */
typedef void     (*dg_init_fn)(void);
typedef void     (*dg_draw_frame_fn)(void);
typedef void     (*dg_sleep_ms_fn)(uint32_t ms);
typedef uint32_t (*dg_get_ticks_ms_fn)(void);
typedef int      (*dg_get_key_fn)(int *pressed, unsigned char *doom_key);
typedef void     (*dg_set_window_title_fn)(const char *title);

/*
 *  Bundle of callbacks you hand to dg_set_callbacks(). Any field except the
 *  ones you truly need may be NULL; the shim null-checks before calling, so a
 *  missing optional callback is simply ignored.
 */
typedef struct {
    dg_init_fn             init;
    dg_draw_frame_fn       draw_frame;
    dg_sleep_ms_fn         sleep_ms;
    dg_get_ticks_ms_fn     get_ticks_ms;
    dg_get_key_fn          get_key;
    dg_set_window_title_fn set_window_title;
} dg_callbacks;

/*
 *  Register your callbacks. Call this BEFORE dg_create(). The struct is copied,
 *  so it does not need to outlive the call -- but the functions it points to
 *  obviously must stay alive for the whole run (keep delegates/closures rooted
 *  in managed languages so the GC does not collect them).
 */
DG_API void dg_set_callbacks(const dg_callbacks *callbacks);

/*
 *  Start the engine. Pass through your program's argv so engine flags work
 *  (e.g. -iwad doom1.wad). Calls your init callback and loads the WAD.
 */
DG_API void dg_create(int argc, char **argv);

/*
 *  Advance one frame. Ends by invoking your draw_frame callback. Call in a loop.
 */
DG_API void dg_tick(void);

/*
 *  Pointer to the shared framebuffer: dg_resx()*dg_resy() pixels, each a 32-bit
 *  0x00RRGGBB value. Valid to read inside your draw_frame callback. Do not free.
 */
DG_API uint32_t *dg_screen_buffer(void);

/* Framebuffer dimensions (compile-time constants in the engine). */
DG_API int dg_resx(void);
DG_API int dg_resy(void);

#ifdef __cplusplus
}
#endif

#endif /* DOOMGENERIC_CAPI_H */
