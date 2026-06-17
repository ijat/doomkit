/* =============================================================================
 *  platform_null.c  --  A headless "port" with ZERO external dependencies.
 * -----------------------------------------------------------------------------
 *  GNU General Public License v2. See LICENSE.
 * =============================================================================
 *
 *  WHAT THIS IS (AND ISN'T)
 *  ------------------------
 *  This file does two jobs at once so that the whole genericdoom contract can
 *  be demonstrated and run with nothing but a C compiler:
 *
 *    1. It is a *port*: it implements the six DG_* callbacks exactly as a real
 *       platform would, using the helper modules (dg_keyqueue, dg_keymap,
 *       dg_palette, dg_framebuffer). Read these to see how the pieces fit.
 *
 *    2. It contains a tiny *fake engine* (`fake_engine_tick`) that stands in
 *       for the real 73k-line DOOM engine. It produces a paletted test image,
 *       converts it into DG_ScreenBuffer the same way the real engine's
 *       i_video.c does, and feeds synthetic key presses through the queue.
 *
 *  Running it (`make run-null`) proves every helper compiles and interoperates,
 *  then writes the final framebuffer to `frame.ppm` so you can SEE the result.
 *
 *  To turn this into a real port you would delete the fake engine and instead
 *  link the actual DOOM engine, calling doomgeneric_Create()/doomgeneric_Tick()
 *  from main(). See examples/template/platform_template.c for that skeleton.
 * ===========================================================================*/

#include "genericdoom/genericdoom.h"
#include "genericdoom/dg_keyqueue.h"
#include "genericdoom/dg_keymap.h"
#include "genericdoom/dg_palette.h"
#include "genericdoom/dg_framebuffer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* The one shared framebuffer required by the contract. A real port does NOT
 * define this (the engine does); we define it here because we are standing in
 * for the engine too. */
pixel_t *DG_ScreenBuffer = NULL;

/* DOOM's native render size. The fake engine draws at this size; the contract
 * framebuffer is the (usually larger) DOOMGENERIC_RESX x DOOMGENERIC_RESY. */
#define SCREENWIDTH  320
#define SCREENHEIGHT 200

/* --- input plumbing: one ring buffer shared by the OS side and DG_GetKey --- */
static dg_keyqueue_t s_keys;

/* --- a clock that starts at the first call so ticks begin near zero --------- */
static uint32_t now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)(ts.tv_sec * 1000u + ts.tv_nsec / 1000000u);
}
static uint32_t s_start_ms = 0;

/* ===========================================================================
 *  THE SIX CONTRACT CALLBACKS
 * ===========================================================================*/

void DG_Init(void)
{
    /* A windowed port would open its window here. We just init our input queue
     * and remember the start time for DG_GetTicksMs. */
    dg_keyqueue_init(&s_keys);
    s_start_ms = now_ms();
    printf("[null] DG_Init: headless, %dx%d framebuffer\n",
           DOOMGENERIC_RESX, DOOMGENERIC_RESY);
}

void DG_DrawFrame(void)
{
    /* A real port copies DG_ScreenBuffer to the screen here. Headless, we have
     * nothing to show on screen each frame; the demo writes the final frame to
     * a file at the end instead. We still drain any pending OS events -- which
     * for us means nothing, since the fake engine injects keys directly. */
}

void DG_SleepMs(uint32_t ms)
{
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (long)(ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
}

uint32_t DG_GetTicksMs(void)
{
    return now_ms() - s_start_ms;
}

int DG_GetKey(int *pressed, unsigned char *doomKey)
{
    /* The whole function is a one-line delegation to the ring buffer, which is
     * the entire point of dg_keyqueue: it already matches this contract. */
    return dg_keyqueue_pop(&s_keys, pressed, doomKey);
}

void DG_SetWindowTitle(const char *title)
{
    printf("[null] window title set to: %s\n", title);
}

/* ===========================================================================
 *  THE FAKE ENGINE  (stands in for real DOOM; uses the same helpers it does)
 * ===========================================================================*/

/* Build a fixed 256-colour palette: a colourful ramp so the test image is easy
 * to see. Real DOOM loads this from the WAD's PLAYPAL lump instead. */
static void build_palette(dg_palette_t *pal)
{
    uint8_t rgb[768];
    for (int i = 0; i < 256; i++) {
        rgb[i * 3 + 0] = (uint8_t)i;             /* red ramps up   */
        rgb[i * 3 + 1] = (uint8_t)((i * 2) & 0xFF); /* green wraps   */
        rgb[i * 3 + 2] = (uint8_t)(255 - i);     /* blue ramps down */
    }
    dg_palette_set(pal, rgb, 0 /* no gamma */);
}

/* Produce one paletted 320x200 frame (a classic XOR texture) and convert it,
 * centred, into the 32-bit DG_ScreenBuffer -- exactly the job i_video.c does. */
static void fake_engine_render(const dg_palette_t *pal, int frame)
{
    static uint8_t doom_frame[SCREENWIDTH * SCREENHEIGHT];

    for (int y = 0; y < SCREENHEIGHT; y++) {
        for (int x = 0; x < SCREENWIDTH; x++) {
            doom_frame[y * SCREENWIDTH + x] = (uint8_t)((x ^ y) + frame);
        }
    }

    /* Work out where to place the 320x200 image inside the framebuffer. */
    dg_fb_layout_t L;
    dg_fb_layout(DOOMGENERIC_RESX, DOOMGENERIC_RESY,
                 SCREENWIDTH, SCREENHEIGHT, 32, 1 /* scaling */, &L);

    /* Start black so the margins are clean, then blit row by row using the
     * byte offsets the layout gave us. */
    memset(DG_ScreenBuffer, 0,
           (size_t)DOOMGENERIC_RESX * DOOMGENERIC_RESY * sizeof(pixel_t));

    unsigned char *out = (unsigned char *)DG_ScreenBuffer + L.y_offset;
    for (int y = 0; y < SCREENHEIGHT; y++) {
        out += L.x_offset;                                  /* left margin   */
        dg_cmap_to_fb32((uint32_t *)out, &doom_frame[y * SCREENWIDTH],
                        SCREENWIDTH, pal, 16, 8, 0);         /* the pixels    */
        out += L.row_stride + L.x_offset_end;               /* right margin  */
    }
}

/* Inject a few key presses, translating SDL-style codes the way a real SDL port
 * would, so DG_GetKey has something to return. */
static void fake_engine_inject_keys(void)
{
    const unsigned int sdl_keys[] = {
        'a',                 /* a letter (default lower-case path) */
        32,                  /* SDL space  -> KEY_USE  */
        0x40000000u | 0x52,  /* SDL up arrow -> KEY_UPARROW */
    };
    for (size_t i = 0; i < sizeof(sdl_keys) / sizeof(sdl_keys[0]); i++) {
        unsigned char dk = dg_keymap_from_sdl(sdl_keys[i]);
        dg_keyqueue_push(&s_keys, 1 /* pressed */, dk);
    }
}

/* ===========================================================================
 *  PPM writer -- dump the final framebuffer so the result is viewable.
 * ===========================================================================*/

static void write_ppm(const char *path)
{
    FILE *f = fopen(path, "wb");
    if (!f) { perror("fopen"); return; }

    fprintf(f, "P6\n%d %d\n255\n", DOOMGENERIC_RESX, DOOMGENERIC_RESY);
    for (int i = 0; i < DOOMGENERIC_RESX * DOOMGENERIC_RESY; i++) {
        uint32_t p = DG_ScreenBuffer[i];          /* 0x00RRGGBB */
        unsigned char rgb[3] = {
            (unsigned char)((p >> 16) & 0xFF),
            (unsigned char)((p >> 8) & 0xFF),
            (unsigned char)(p & 0xFF),
        };
        fwrite(rgb, 1, 3, f);
    }
    fclose(f);
}

/* ===========================================================================
 *  main -- the loop a real port runs, but driving the fake engine.
 * ===========================================================================*/

int main(void)
{
    /* The engine normally allocates this inside doomgeneric_Create(); we do it
     * by hand because we are the engine here. */
    DG_ScreenBuffer = malloc((size_t)DOOMGENERIC_RESX * DOOMGENERIC_RESY *
                             sizeof(pixel_t));
    if (!DG_ScreenBuffer) { fprintf(stderr, "out of memory\n"); return 1; }

    DG_Init();                       /* would be called by doomgeneric_Create */
    DG_SetWindowTitle("genericdoom-cleancode null demo");

    dg_palette_t pal;
    build_palette(&pal);

    /* Run a handful of "ticks". A real port would loop forever. */
    const int frames = 3;
    for (int frame = 0; frame < frames; frame++) {
        fake_engine_render(&pal, frame * 40);   /* engine fills DG_ScreenBuffer */
        fake_engine_inject_keys();              /* OS delivers some keys        */
        DG_DrawFrame();                          /* port presents the frame      */

        /* Drain input the way the engine's i_input.c does. */
        int pressed; unsigned char dk;
        while (DG_GetKey(&pressed, &dk)) {
            printf("[engine] key event: pressed=%d doomKey=0x%02X\n", pressed, dk);
        }

        DG_SleepMs(5);
        printf("[loop] frame %d done at t=%u ms\n", frame, DG_GetTicksMs());
    }

    write_ppm("frame.ppm");
    printf("[null] wrote frame.ppm (%dx%d). Open it to see the test image.\n",
           DOOMGENERIC_RESX, DOOMGENERIC_RESY);

    free(DG_ScreenBuffer);
    return 0;
}
