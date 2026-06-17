/* =============================================================================
 *  doomkit.h  --  The DOOM porting contract.
 * -----------------------------------------------------------------------------
 *  Derived from doomgeneric (https://github.com/ozkl/doomgeneric), which is
 *  itself a thin shim over id Software's DOOM engine.
 *
 *  Copyright (C) 1993-1996 id Software, Inc.
 *  Copyright (C) doomgeneric authors.
 *  This program is free software distributed under the GNU General Public
 *  License version 2. See the LICENSE file at the root of this package.
 * =============================================================================
 *
 *  WHAT THIS FILE IS
 *  -----------------
 *  This single header is the *entire* surface a new platform has to care about
 *  to run DOOM. The DOOM engine itself is ~73,000 lines of battle-tested C, but
 *  you never touch it. Instead, the engine talks to the outside world through
 *  the small contract below.
 *
 *  THE CONTRACT HAS THREE PARTS
 *  ----------------------------
 *    1. A shared framebuffer the engine draws into:   DG_ScreenBuffer
 *    2. Two lifecycle functions the engine provides:  doomgeneric_Create / _Tick
 *    3. Six callbacks YOU implement for your platform: the DG_* functions
 *
 *  Think of it like a power socket: the engine is the appliance, and the six
 *  DG_* callbacks are the pins you wire up to your specific wall. Implement the
 *  six pins, call Create() once and Tick() in a loop, and DOOM runs.
 *
 *  See docs/CONTRACT.md for the precise semantics of every function, and
 *  docs/PORTING.md for a step-by-step walkthrough.
 * ===========================================================================*/

#ifndef DOOMKIT_H
#define DOOMKIT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* -----------------------------------------------------------------------------
 *  Screen resolution
 * -----------------------------------------------------------------------------
 *  DOOM renders internally at 320x200 and the engine scales that up into this
 *  output framebuffer. These default to 640x400 (a clean 2x of 320x200). You
 *  may override them at compile time (e.g. -DDOOMGENERIC_RESX=1280) to match
 *  your display, but keeping a 16:10 ratio avoids distortion.
 * ---------------------------------------------------------------------------*/
#ifndef DOOMGENERIC_RESX
#define DOOMGENERIC_RESX 640
#endif

#ifndef DOOMGENERIC_RESY
#define DOOMGENERIC_RESY 400
#endif

/* -----------------------------------------------------------------------------
 *  Pixel format of the output framebuffer
 * -----------------------------------------------------------------------------
 *  By default each pixel is a 32-bit value laid out as 0x00RRGGBB (one byte
 *  per channel, the top byte unused). This is the format almost every modern
 *  platform wants, so most ports can hand DG_ScreenBuffer straight to their
 *  display API.
 *
 *  If you build with -DCMAP256 the engine instead emits 8-bit palette indices,
 *  which is useful for genuinely tiny or paletted displays. The vast majority
 *  of ports leave this off and work with 32-bit pixels.
 * ---------------------------------------------------------------------------*/
#ifdef CMAP256
typedef uint8_t pixel_t;   /* 8-bit palette index */
#else
typedef uint32_t pixel_t;  /* 0x00RRGGBB */
#endif

/* -----------------------------------------------------------------------------
 *  DG_ScreenBuffer -- the one piece of shared memory in the whole contract.
 * -----------------------------------------------------------------------------
 *  The engine allocates this buffer inside doomgeneric_Create() and fills it
 *  with a finished frame just before calling your DG_DrawFrame(). It is a flat,
 *  row-major array of exactly (DOOMGENERIC_RESX * DOOMGENERIC_RESY) pixels.
 *
 *  Your job in DG_DrawFrame() is simply to copy these pixels onto the screen.
 *  You do not own this buffer and must not free it; the engine manages its
 *  lifetime.
 * ---------------------------------------------------------------------------*/
extern pixel_t *DG_ScreenBuffer;

/* =============================================================================
 *  LIFECYCLE -- provided BY the engine, called BY your platform's main()
 * ===========================================================================*/

/*
 *  doomgeneric_Create -- one-time engine startup.
 *
 *  Call this exactly once, before the game loop. It:
 *    - stashes argc/argv so the engine can read command-line flags
 *      (e.g. -iwad doom1.wad, -window),
 *    - allocates DG_ScreenBuffer,
 *    - calls your DG_Init() so you can open a window / framebuffer,
 *    - boots the DOOM engine (loads the WAD, sets up the renderer, etc.).
 *
 *  Pass your program's argc/argv straight through.
 */
void doomgeneric_Create(int argc, char **argv);

/*
 *  doomgeneric_Tick -- advance the game by one frame.
 *
 *  Call this repeatedly in your main loop. Each call:
 *    - reads input (pulling events from your DG_GetKey),
 *    - runs game logic for the elapsed time,
 *    - renders one frame into DG_ScreenBuffer,
 *    - calls your DG_DrawFrame() to present it.
 *
 *  It does not sleep on its own; pace your loop using DG_GetTicksMs/DG_SleepMs
 *  (or rely on your platform's vsync) so the game runs at ~35 fps.
 */
void doomgeneric_Tick(void);

/* =============================================================================
 *  PLATFORM CALLBACKS -- you implement these (this is the whole port)
 * -----------------------------------------------------------------------------
 *  Put your implementations in one file, e.g. platform_yourthing.c. See the
 *  examples/ folder for SDL, a headless "null" port, and a blank template.
 * ===========================================================================*/

/*
 *  DG_Init -- set up your platform's display and input.
 *
 *  Called once from inside doomgeneric_Create(). Open your window, create a
 *  texture/framebuffer of DOOMGENERIC_RESX x DOOMGENERIC_RESY, and initialise
 *  whatever you need to read the keyboard. Do NOT draw a frame here; the first
 *  real frame arrives via DG_DrawFrame().
 */
void DG_Init(void);

/*
 *  DG_DrawFrame -- present a finished frame.
 *
 *  Called once per doomgeneric_Tick(). DG_ScreenBuffer already holds the
 *  completed frame in the agreed pixel format. Copy/blit it to the screen.
 *  This is also the natural place to pump your OS event queue and feed any
 *  key events into the queue that DG_GetKey() reads from.
 */
void DG_DrawFrame(void);

/*
 *  DG_SleepMs -- block the calling thread for `ms` milliseconds.
 *
 *  The engine uses this to avoid busy-waiting when it is ahead of schedule.
 *  A plain sleep is fine.
 */
void DG_SleepMs(uint32_t ms);

/*
 *  DG_GetTicksMs -- milliseconds elapsed since the program started.
 *
 *  Used as DOOM's wall clock to keep a steady ~35 Hz simulation. The absolute
 *  zero point does not matter as long as it increases monotonically and is
 *  measured in milliseconds.
 */
uint32_t DG_GetTicksMs(void);

/*
 *  DG_GetKey -- hand the engine the next keyboard event, if any.
 *
 *  The engine calls this in a loop, draining one event per call:
 *    - return 0 when you have no more events queued,
 *    - return 1 when you produced an event, and write:
 *        *pressed  -> 1 for key-down, 0 for key-up
 *        *doomKey  -> the DOOM key code (see dg_keys.h)
 *
 *  Your platform's raw key codes must be translated to DOOM key codes first;
 *  dg_keymap.h does exactly that, and dg_keyqueue.h gives you a ready-made
 *  ring buffer to hold events between OS callbacks and this function.
 */
int DG_GetKey(int *pressed, unsigned char *doomKey);

/*
 *  DG_SetWindowTitle -- update the window title (optional).
 *
 *  DOOM derives a title from the loaded WAD and calls this once. If your
 *  platform has no concept of a window title, implement it as an empty
 *  function.
 */
void DG_SetWindowTitle(const char *title);

#ifdef __cplusplus
}
#endif

#endif /* DOOMKIT_H */
