/* =============================================================================
 *  platform_null_engine.c  --  A *real* headless port: actual DOOM, no display.
 * -----------------------------------------------------------------------------
 *  GNU General Public License v2. See LICENSE.
 * =============================================================================
 *
 *  WHAT THIS IS (AND HOW IT DIFFERS FROM platform_null.c)
 *  ------------------------------------------------------
 *  platform_null.c proves the *contract* with a fake engine and no external
 *  dependencies -- a great first read, but the pixels come from a stand-in.
 *
 *  This file is the next notch up: it is the SAME six DG_* callbacks, written
 *  for *no screen at all*, but linked against the **real ~73k-line DOOM engine**
 *  (doomgeneric_Create / doomgeneric_Tick). There is no fake engine here; the
 *  engine itself fills DG_ScreenBuffer. We simply:
 *
 *    1. start the engine,
 *    2. tick it for ~10 seconds of wall-clock time (the title screen and the
 *       attract-mode demo loops render with no input), then
 *    3. write the final framebuffer to frame_engine.ppm so you can SEE that
 *       real DOOM rendered without ever opening a window.
 *
 *  Unlike platform_null.c, this needs two extra things at build time:
 *    - the upstream DOOM engine sources (ENGINE=...), which provide
 *      doomgeneric.c (it defines DG_ScreenBuffer and the Create/Tick loop), and
 *    - a WAD (game data) to pass on the command line, e.g. -iwad doom1.wad.
 *
 *  Build & run it with `make run-null-engine ENGINE=... WAD=...`.
 * ===========================================================================*/

#include "doomkit/doomkit.h"   /* the contract: DG_*, doomgeneric_Create/Tick,
                                  DG_ScreenBuffer, DOOMGENERIC_RESX/RESY      */

#include <stdio.h>
#include <stdint.h>
#include <time.h>

/* NOTE: we do NOT define DG_ScreenBuffer here. The real engine (doomgeneric.c)
 * owns it and allocates it inside doomgeneric_Create(). That is the whole point
 * of this example -- a genuine engine, not a stand-in. */

/* How long to run the engine before dumping a frame. */
#define RUN_MS 10000u

/* ---------------------------------------------------------------------------
 *  A monotonic millisecond clock, zeroed at the first call so engine ticks
 *  begin near zero. The engine uses DG_GetTicksMs for ALL its timing, so this
 *  must be real and monotonic -- and because it is wall-clock, our 10-second
 *  budget below is 10 real seconds of gameplay.
 * ------------------------------------------------------------------------- */
static uint32_t now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)(ts.tv_sec * 1000u + ts.tv_nsec / 1000000u);
}
static uint32_t s_start_ms = 0;

/* ===========================================================================
 *  THE SIX CONTRACT CALLBACKS  (headless: no window, no input)
 * ===========================================================================*/

void DG_Init(void)
{
    /* A windowed port opens its window here. Headless, there is nothing to
     * open; we only anchor the clock. */
    s_start_ms = now_ms();
    printf("[null-engine] DG_Init: headless, %dx%d framebuffer\n",
           DOOMGENERIC_RESX, DOOMGENERIC_RESY);
}

void DG_DrawFrame(void)
{
    /* A real port copies DG_ScreenBuffer to the screen here. Headless, we have
     * nowhere to put it each frame -- main() writes the final frame to a file
     * once the run is over. There is also no OS event queue to pump. */
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
    /* No keyboard in a headless run: the engine just gets "no events", so it
     * sits on the title screen and then plays its attract-mode demos. */
    (void)pressed;
    (void)doomKey;
    return 0;
}

void DG_SetWindowTitle(const char *title)
{
    printf("[null-engine] window title set to: %s\n", title);
}

/* ===========================================================================
 *  PPM writer -- dump the final framebuffer so the result is viewable.
 *  (Identical to platform_null.c's, on purpose: the output format is the same;
 *  only the source of the pixels -- the real engine -- has changed.)
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
 *  main -- the loop a real port runs, driving the REAL engine.
 * ===========================================================================*/

int main(int argc, char **argv)
{
    /* doomgeneric_Create() saves argv (so -iwad etc. are honoured), allocates
     * DG_ScreenBuffer, calls our DG_Init(), runs D_DoomMain() to load the WAD,
     * and renders the first frame. It returns to us -- then we drive the loop,
     * exactly as examples/platforms/template/platform_template.c does, except we
     * stop after RUN_MS instead of looping forever. */
    doomgeneric_Create(argc, argv);

    unsigned long ticks = 0;
    while (DG_GetTicksMs() < RUN_MS) {
        doomgeneric_Tick();   /* engine advances one frame into DG_ScreenBuffer */
        ticks++;
    }

    write_ppm("frame_engine.ppm");
    printf("[null-engine] ran %lu ticks in ~%u ms, wrote frame_engine.ppm "
           "(%dx%d). Open it to see real DOOM, rendered headless.\n",
           ticks, RUN_MS, DOOMGENERIC_RESX, DOOMGENERIC_RESY);

    return 0;
}
