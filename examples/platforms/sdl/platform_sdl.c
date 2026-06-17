/* =============================================================================
 *  platform_sdl.c  --  Reference port for SDL2.
 * -----------------------------------------------------------------------------
 *  GNU General Public License v2. See LICENSE.
 * =============================================================================
 *
 *  This is the canonical "real" port: SDL2 gives us a window, a clock, a sleep,
 *  and keyboard events, and we wire those into the six DG_* callbacks. Compare
 *  it with platform_null.c -- the structure is identical, only the backend
 *  differs.
 *
 *  It reuses the shared helpers instead of re-implementing them:
 *    - dg_keymap_from_sdl() turns SDL key symbols into DOOM key codes,
 *    - dg_keyqueue holds events between SDL's callbacks and DG_GetKey().
 *
 *  BUILDING (not built by `make` here, because it needs SDL2 installed):
 *    cc -Iinclude $(sdl2-config --cflags) \
 *       examples/sdl/platform_sdl.c \
 *       src/dg_keyqueue.c src/dg_keymap.c \
 *       <the DOOM engine .c files> \
 *       $(sdl2-config --libs) -o doom
 *
 *  The DOOM engine sources (d_main.c, r_*.c, p_*.c, ...) provide
 *  doomgeneric_Create()/doomgeneric_Tick(); drop them in from upstream
 *  doomgeneric. See docs/PORTING.md.
 * ===========================================================================*/

#include "doomkit/doomkit.h"
#include "doomkit/dg_keyqueue.h"
#include "doomkit/dg_keymap.h"

#include <SDL.h>
#include <stdio.h>

static SDL_Window   *s_window   = NULL;
static SDL_Renderer *s_renderer = NULL;
static SDL_Texture  *s_texture  = NULL;

/* Events arrive while we pump SDL inside DG_DrawFrame; DG_GetKey reads them. */
static dg_keyqueue_t s_keys;

/* -------------------------------------------------------------------------- */

void DG_Init(void)
{
    dg_keyqueue_init(&s_keys);

    SDL_Init(SDL_INIT_VIDEO);

    s_window = SDL_CreateWindow(
        "DOOM",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        DOOMGENERIC_RESX, DOOMGENERIC_RESY, SDL_WINDOW_SHOWN);

    s_renderer = SDL_CreateRenderer(s_window, -1, SDL_RENDERER_ACCELERATED);

    /* RGB888 matches our 0x00RRGGBB pixel layout, so we can hand SDL the
     * framebuffer with no conversion. */
    s_texture = SDL_CreateTexture(
        s_renderer, SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_STREAMING,
        DOOMGENERIC_RESX, DOOMGENERIC_RESY);
}

/* Pull every pending SDL event, translate key codes, and queue key up/downs. */
static void pump_events(void)
{
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        switch (e.type) {
            case SDL_QUIT:
                exit(0);
            case SDL_KEYDOWN:
                dg_keyqueue_push(&s_keys, 1, dg_keymap_from_sdl(e.key.keysym.sym));
                break;
            case SDL_KEYUP:
                dg_keyqueue_push(&s_keys, 0, dg_keymap_from_sdl(e.key.keysym.sym));
                break;
            default:
                break;
        }
    }
}

void DG_DrawFrame(void)
{
    /* Upload the finished frame and present it. */
    SDL_UpdateTexture(s_texture, NULL, DG_ScreenBuffer,
                      DOOMGENERIC_RESX * sizeof(pixel_t));
    SDL_RenderClear(s_renderer);
    SDL_RenderCopy(s_renderer, s_texture, NULL, NULL);
    SDL_RenderPresent(s_renderer);

    /* DG_DrawFrame is the once-per-tick heartbeat, so it is the natural place
     * to service the OS event queue. */
    pump_events();
}

void DG_SleepMs(uint32_t ms)
{
    SDL_Delay(ms);
}

uint32_t DG_GetTicksMs(void)
{
    return SDL_GetTicks();
}

int DG_GetKey(int *pressed, unsigned char *doomKey)
{
    return dg_keyqueue_pop(&s_keys, pressed, doomKey);
}

void DG_SetWindowTitle(const char *title)
{
    if (s_window) {
        SDL_SetWindowTitle(s_window, title);
    }
}

/* -------------------------------------------------------------------------- */

int main(int argc, char **argv)
{
    doomgeneric_Create(argc, argv);
    for (;;) {
        doomgeneric_Tick();
    }
    return 0;
}
