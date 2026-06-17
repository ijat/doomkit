/* =============================================================================
 *  examples/languages/c/main.c
 * -----------------------------------------------------------------------------
 *  Drive the genericdoom SHARED LIBRARY from plain C, using the registration
 *  C API (bindings/doomgeneric_capi.h). GPLv2. See LICENSE.
 * =============================================================================
 *
 *  This is the simplest possible host: it implements the six callbacks, hands
 *  them to the library with dg_set_callbacks(), then runs the create/tick loop.
 *  It is headless -- DG_DrawFrame just saves one frame to a PPM so you can see
 *  that real pixels came out -- so it runs anywhere with no GUI toolkit.
 *
 *  BUILD (after building libdoomgeneric -- see bindings/README.md):
 *      cc -I../../../bindings main.c -L<dir-with-lib> -ldoomgeneric -o doomdemo
 *  RUN:
 *      ./doomdemo -iwad doom1.wad
 * ===========================================================================*/

#include "doomgeneric_capi.h"

#include <stdio.h>
#include <stdint.h>
#include <time.h>

static int      g_frame = 0;
static uint32_t g_start_ms = 0;

static uint32_t now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)(ts.tv_sec * 1000u + ts.tv_nsec / 1000000u);
}

/* ---- the six callbacks --------------------------------------------------- */

static void cb_init(void)
{
    g_start_ms = now_ms();
    printf("[c] init: framebuffer %dx%d\n", dg_resx(), dg_resy());
}

static void cb_draw_frame(void)
{
    /* Save frame #100 to prove pixels are flowing, then we can stop. */
    if (++g_frame == 100) {
        FILE *f = fopen("frame.ppm", "wb");
        if (f) {
            int w = dg_resx(), h = dg_resy();
            uint32_t *buf = dg_screen_buffer();
            fprintf(f, "P6\n%d %d\n255\n", w, h);
            for (int i = 0; i < w * h; i++) {
                uint32_t p = buf[i];                 /* 0x00RRGGBB */
                unsigned char rgb[3] = { (p >> 16) & 0xFF, (p >> 8) & 0xFF, p & 0xFF };
                fwrite(rgb, 1, 3, f);
            }
            fclose(f);
            printf("[c] wrote frame.ppm at frame %d\n", g_frame);
        }
    }
}

static void     cb_sleep_ms(uint32_t ms) { struct timespec t = { ms / 1000, (long)(ms % 1000) * 1000000L }; nanosleep(&t, NULL); }
static uint32_t cb_get_ticks_ms(void)    { return now_ms() - g_start_ms; }
static int      cb_get_key(int *pressed, unsigned char *key) { (void)pressed; (void)key; return 0; /* no input */ }
static void     cb_set_title(const char *t) { printf("[c] title: %s\n", t); }

/* ---- main ---------------------------------------------------------------- */

int main(int argc, char **argv)
{
    dg_callbacks cb = {
        .init             = cb_init,
        .draw_frame       = cb_draw_frame,
        .sleep_ms         = cb_sleep_ms,
        .get_ticks_ms     = cb_get_ticks_ms,
        .get_key          = cb_get_key,
        .set_window_title = cb_set_title,
    };
    dg_set_callbacks(&cb);          /* register BEFORE create */

    dg_create(argc, argv);          /* boots engine, calls cb_init, loads WAD */
    for (int i = 0; i < 200; i++) { /* a real app loops forever */
        dg_tick();
    }
    printf("[c] done.\n");
    return 0;
}
