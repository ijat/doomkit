/* =============================================================================
 *  examples/wasm/platform_wasm.c  --  Run DOOM in a web browser via Emscripten.
 * -----------------------------------------------------------------------------
 *  GPLv2. See LICENSE.
 * =============================================================================
 *
 *  This is a normal doomkit platform port (it implements the six DG_* callbacks
 *  directly, like examples/sdl and examples/null) -- it just targets the
 *  browser. Emscripten compiles this file + the DOOM engine + dg_keyqueue into
 *  one WebAssembly module; the surrounding index.html provides the <canvas>,
 *  keyboard handling, and the WAD file.
 *
 *  The three browser-specific ideas:
 *    1. DG_DrawFrame hands the framebuffer pointer to a JS function that copies
 *       it onto a <canvas> (via EM_ASM, Emscripten's inline-JavaScript macro).
 *    2. There is no blocking loop. The browser drives frames, so we register
 *       doomgeneric_Tick with emscripten_set_main_loop() and never sleep.
 *    3. Keyboard events come FROM JavaScript: index.html calls the exported
 *       wasm_push_key() function, which queues into the same dg_keyqueue the
 *       desktop ports use.
 *
 *  Build: see README.md (emcc), or `make wasm` from the project root.
 * ===========================================================================*/

#include "doomkit/doomkit.h"        /* the contract: DG_*, DG_ScreenBuffer, RES */
#include "doomkit/dg_keyqueue.h"     /* reuse the tested input ring buffer       */

#include <emscripten.h>
#include <stdint.h>

/* Events flow JS -> wasm_push_key -> this queue -> DG_GetKey -> engine. */
static dg_keyqueue_t s_keys;

/* ---- the six contract callbacks ------------------------------------------ */

void DG_Init(void)
{
    dg_keyqueue_init(&s_keys);
}

void DG_DrawFrame(void)
{
    /* Call the JS function dgDrawFrame(ptr, width, height) defined in
     * index.html. It reads `width*height` 32-bit 0x00RRGGBB pixels starting at
     * `ptr` out of the WASM heap and paints them onto the canvas. */
    EM_ASM({
        dgDrawFrame($0, $1, $2);
    }, DG_ScreenBuffer, DOOMGENERIC_RESX, DOOMGENERIC_RESY);
}

void DG_SleepMs(uint32_t ms)
{
    /* No-op: a browser tab must never block. The page's animation-frame loop
     * paces us instead. */
    (void)ms;
}

uint32_t DG_GetTicksMs(void)
{
    /* emscripten_get_now() is high-resolution milliseconds since page load. */
    return (uint32_t)emscripten_get_now();
}

int DG_GetKey(int *pressed, unsigned char *doom_key)
{
    return dg_keyqueue_pop(&s_keys, pressed, doom_key);
}

void DG_SetWindowTitle(const char *title)
{
    EM_ASM({ document.title = UTF8ToString($0); }, title);
}

/* ---- exported to JavaScript ---------------------------------------------- *
 *  index.html maps browser KeyboardEvents to DOOM key codes (see dg_keys.h)
 *  and calls Module._wasm_push_key(pressed, doomKey) for each one.
 * -------------------------------------------------------------------------- */
EMSCRIPTEN_KEEPALIVE
void wasm_push_key(int pressed, int doom_key)
{
    dg_keyqueue_push(&s_keys, pressed, (unsigned char)doom_key);
}

/* ---- entry point --------------------------------------------------------- *
 *  Called from JS via Module.callMain([...]) AFTER the WAD has been written
 *  into Emscripten's virtual filesystem. We boot the engine and then hand the
 *  per-frame tick to the browser.
 * -------------------------------------------------------------------------- */
int main(int argc, char **argv)
{
    doomgeneric_Create(argc, argv);

    /* fps=0 -> use requestAnimationFrame; simulate_infinite_loop=1 -> keep the
     * module alive after main() returns. The engine self-paces to ~35 Hz using
     * DG_GetTicksMs, so calling Tick at the browser's ~60 Hz is fine. */
    emscripten_set_main_loop(doomgeneric_Tick, 0, 1);
    return 0;
}
