/* =============================================================================
 *  platform_terminal.c  --  Playable DOOM rendered as colored ASCII in a TTY.
 * -----------------------------------------------------------------------------
 *  GNU General Public License v2. See LICENSE.
 * =============================================================================
 *
 *  WHAT THIS IS
 *  ------------
 *  The yes-it-really-runs-DOOM-in-a-terminal port. It is the SAME six DG_*
 *  callbacks as every other port, linked against the **real ~73k-line DOOM
 *  engine** (doomgeneric_Create / doomgeneric_Tick) -- but the "screen" is your
 *  terminal window and the "keyboard" is raw stdin. No SDL, no X11, no GPU; only
 *  POSIX termios + ioctl, which every Unix already has.
 *
 *  It is closest in spirit to platform_null_engine.c (same real engine, no GUI
 *  toolkit), except instead of dumping one frame to a .ppm it draws every frame
 *  to the terminal and lets you actually play.
 *
 *  HOW THE PICTURE IS MADE (DG_DrawFrame)
 *  --------------------------------------
 *  The engine hands us a 640x400 array of 0x00RRGGBB pixels in DG_ScreenBuffer.
 *  A terminal cell is much bigger than a pixel and is taller than it is wide
 *  (~2:1), so each frame we:
 *    1. ask the terminal how many columns x rows it currently has (ioctl),
 *    2. average each source block down to one cell,
 *    3. pick an ASCII character from a brightness ramp for that cell's luma,
 *    4. set the cell's 24-bit ANSI foreground colour to the block's mean colour,
 *    5. write the whole screen in ONE write() after homing the cursor, so there
 *       is no tearing and no flicker.
 *
 *  THE ONE GENUINELY TRICKY BIT: KEY RELEASES (DG_GetKey)
 *  ------------------------------------------------------
 *  A real keyboard sends a "down" event AND an "up" event, and DOOM needs both
 *  (hold to keep walking, release to stop). A terminal only ever sends the
 *  *character* -- there is no key-up at all. So we fake it: when a byte arrives
 *  we queue a press immediately and schedule a matching release a few frames
 *  later. The effect is tap-to-step movement: each keypress nudges you for a
 *  fraction of a second. Hold the key and the terminal's own auto-repeat keeps
 *  re-pressing it, so you walk continuously. This is the standard compromise for
 *  terminal ports and the only part of this file that isn't obvious.
 *
 *  BUILD & RUN
 *  -----------
 *  Like platform_null_engine.c this needs the upstream engine sources and a WAD:
 *      make run-terminal ENGINE=/path/to/doomgeneric/doomgeneric WAD=/path/doom1.wad
 *  Use a truecolor terminal at roughly 80x50 or larger. Quit with Ctrl-C (the
 *  terminal is always restored on the way out) or by quitting from DOOM's menu.
 * ===========================================================================*/

#include "doomkit/doomkit.h"   /* the contract: DG_*, doomgeneric_Create/Tick,
                                  DG_ScreenBuffer, DOOMGENERIC_RESX/RESY      */
#include "doomkit/dg_keys.h"   /* KEY_* codes we translate raw bytes into     */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include <termios.h>
#include <sys/ioctl.h>

/* We do NOT define DG_ScreenBuffer: the real engine (doomgeneric.c) owns it and
 * allocates it inside doomgeneric_Create(). Same rule as platform_null_engine.c. */

/* ---------------------------------------------------------------------------
 *  Monotonic millisecond clock, zeroed on first use so engine ticks start near
 *  zero. The engine drives ALL its timing through DG_GetTicksMs, so this must
 *  be real and monotonic. (Identical to platform_null_engine.c.)
 * ------------------------------------------------------------------------- */
static uint32_t now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)(ts.tv_sec * 1000u + ts.tv_nsec / 1000000u);
}
static uint32_t s_start_ms = 0;

/* ===========================================================================
 *  TERMINAL SETUP / TEARDOWN
 * ---------------------------------------------------------------------------
 *  We flip the terminal into "raw" mode so keystrokes reach us one byte at a
 *  time, unbuffered and not echoed. That is great for a game and terrible for
 *  the shell you came from, so we stash the original settings and restore them
 *  on EVERY exit path -- normal return, exit(), and Ctrl-C -- otherwise a crash
 *  would leave the user with an invisible, non-echoing prompt.
 * ===========================================================================*/

static struct termios s_orig_termios;
static int            s_termios_saved = 0;

/* ANSI escapes used below:
 *   \033[2J  clear screen        \033[H   cursor to home (top-left)
 *   \033[?25l hide cursor        \033[?25h show cursor
 *   \033[0m  reset attributes                                              */
static void term_restore(void)
{
    if (s_termios_saved)
        tcsetattr(STDIN_FILENO, TCSANOW, &s_orig_termios);
    /* show cursor, reset colours, leave a clean line for the next prompt */
    const char *cleanup = "\033[0m\033[?25h\033[H\033[2J";
    ssize_t r = write(STDOUT_FILENO, cleanup, strlen(cleanup));
    (void)r;
}

static void on_signal(int sig)
{
    term_restore();
    /* Re-raise with the default handler so the exit status is correct and the
     * shell knows we died from a signal. */
    signal(sig, SIG_DFL);
    raise(sig);
}

static void term_setup(void)
{
    if (tcgetattr(STDIN_FILENO, &s_orig_termios) == 0)
        s_termios_saved = 1;

    struct termios raw = s_orig_termios;
    /* raw input: no canonical line buffering, no echo, no signal/flow munging */
    raw.c_lflag &= ~(unsigned)(ICANON | ECHO | ISIG | IEXTEN);
    raw.c_iflag &= ~(unsigned)(IXON | ICRNL | INPCK | ISTRIP);
    raw.c_cc[VMIN]  = 0;   /* read() returns immediately... */
    raw.c_cc[VTIME] = 0;   /* ...even with zero bytes available (non-blocking) */
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);

    /* Belt and braces: also mark the fd non-blocking, so our drain loop in
     * DG_GetKey never stalls the engine waiting for input. */
    int fl = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, fl | O_NONBLOCK);

    atexit(term_restore);
    signal(SIGINT,  on_signal);
    signal(SIGTERM, on_signal);

    const char *init = "\033[2J\033[H\033[?25l";   /* clear, home, hide cursor */
    ssize_t r = write(STDOUT_FILENO, init, strlen(init));
    (void)r;
}

/* ===========================================================================
 *  THE INPUT QUEUE  (with synthetic key-releases -- see file header)
 * ---------------------------------------------------------------------------
 *  Raw bytes from the terminal become DOOM key events here. Each keypress
 *  pushes a "down" event right away and remembers to push a matching "up" event
 *  a little later (RELEASE_AFTER_MS). DG_GetKey hands events to the engine and
 *  fires due releases on the way through.
 * ===========================================================================*/

#define EVT_CAP          64
#define RELEASE_AFTER_MS 120   /* how long a tapped key counts as "held" */

typedef struct { int pressed; unsigned char key; } key_evt_t;

/* Ring buffer of ready-to-deliver events (presses, and releases once due). */
static key_evt_t s_evt[EVT_CAP];
static int       s_evt_head = 0, s_evt_tail = 0;

/* Parallel list of releases waiting for their time to come. */
static struct { unsigned char key; uint32_t due_ms; } s_pending[EVT_CAP];
static int s_pending_n = 0;

static void evt_push(int pressed, unsigned char key)
{
    int next = (s_evt_tail + 1) % EVT_CAP;
    if (next == s_evt_head) return;        /* full: drop (only under key spam) */
    s_evt[s_evt_tail].pressed = pressed;
    s_evt[s_evt_tail].key     = key;
    s_evt_tail = next;
}

/* Register a press now and schedule its release for later. */
static void key_tap(unsigned char key)
{
    evt_push(1, key);
    if (s_pending_n < EVT_CAP)
        s_pending[s_pending_n].key    = key,
        s_pending[s_pending_n].due_ms = now_ms() + RELEASE_AFTER_MS,
        s_pending_n++;
}

/* Move any releases whose time has arrived into the deliverable queue. */
static void flush_due_releases(void)
{
    uint32_t t = now_ms();
    int i = 0;
    while (i < s_pending_n) {
        if ((int32_t)(t - s_pending[i].due_ms) >= 0) {
            evt_push(0, s_pending[i].key);
            s_pending[i] = s_pending[--s_pending_n];   /* swap-remove */
        } else {
            i++;
        }
    }
}

/* -------------------------------------------------------------------------
 *  Translate one logical keystroke (already past any escape sequence) into a
 *  DOOM key code, or 0 to ignore it.
 * ----------------------------------------------------------------------- */
static unsigned char doomkey_for_char(unsigned char c)
{
    switch (c) {
        case ' ':  return KEY_USE;        /* open doors / use switches      */
        case '\r':
        case '\n': return KEY_ENTER;      /* confirm menu selection         */
        case '\t': return KEY_TAB;        /* automap                        */
        case 127:
        case 8:    return KEY_BACKSPACE;
        case '-':  return KEY_MINUS;
        case '=':
        case '+':  return KEY_EQUALS;

        /* Movement on a keyboard with no arrow keys handy (WASD), plus the
         * two actions on easy-to-reach keys. */
        case 'w': case 'W': return KEY_UPARROW;
        case 's': case 'S': return KEY_DOWNARROW;
        case 'a': case 'A': return KEY_LEFTARROW;
        case 'd': case 'D': return KEY_RIGHTARROW;
        case ',':           return KEY_STRAFE_L;
        case '.':           return KEY_STRAFE_R;
        case 'f': case 'F':
        case 'x': case 'X': return KEY_FIRE;      /* shoot */
        case 'e': case 'E': return KEY_USE;

        case 'y': case 'Y': return 'y';    /* menu yes/no prompts */
        case 'n': case 'N': return 'n';
        default:
            /* Pass printable ASCII straight through (digits select menu items,
             * etc.); DOOM uses the literal ASCII value as the key code. */
            if (c >= 0x20 && c < 0x7f) return c;
            return 0;
    }
}

/* Read everything waiting on stdin and turn it into taps. Handles the CSI
 * arrow-key sequences (ESC [ A/B/C/D) and a bare ESC (the menu/back key). */
static void pump_terminal_input(void)
{
    unsigned char buf[64];
    ssize_t n = read(STDIN_FILENO, buf, sizeof buf);
    if (n <= 0) return;

    for (ssize_t i = 0; i < n; i++) {
        unsigned char c = buf[i];

        if (c == 0x1b /* ESC */) {
            /* Could be a real Escape, or the lead-in of an arrow key:
             *   ESC '[' 'A'    up        ESC '[' 'B'   down
             *   ESC '[' 'C'    right     ESC '[' 'D'   left            */
            if (i + 2 < n && buf[i + 1] == '[') {
                unsigned char a = buf[i + 2];
                unsigned char k = 0;
                if      (a == 'A') k = KEY_UPARROW;
                else if (a == 'B') k = KEY_DOWNARROW;
                else if (a == 'C') k = KEY_RIGHTARROW;
                else if (a == 'D') k = KEY_LEFTARROW;
                if (k) { key_tap(k); i += 2; continue; }
            }
            key_tap(KEY_ESCAPE);
            continue;
        }

        unsigned char k = doomkey_for_char(c);
        if (k) key_tap(k);
    }
}

/* ===========================================================================
 *  THE SIX CONTRACT CALLBACKS
 * ===========================================================================*/

void DG_Init(void)
{
    term_setup();
    s_start_ms = now_ms();
}

/* ---------------------------------------------------------------------------
 *  DG_DrawFrame -- downsample DG_ScreenBuffer to colored ASCII and present it.
 * ------------------------------------------------------------------------- */

/* Brightness ramp from darkest to brightest. Index by luma (0..255). */
static const char  RAMP[]   = " .:-=+*#%@";
static const int   RAMP_LEN = (int)sizeof(RAMP) - 2;   /* exclude NUL, 0-based */

/* One reusable output buffer for the whole frame; grown as needed. Worst case a
 * cell costs ~20 bytes (a truecolor SGR escape + the glyph), so we size to the
 * terminal's cell count times a generous per-cell budget. */
static char  *s_out     = NULL;
static size_t s_out_cap = 0;

static void ensure_out(size_t need)
{
    if (need <= s_out_cap) return;
    s_out_cap = need * 2;
    s_out = (char *)realloc(s_out, s_out_cap);
}

void DG_DrawFrame(void)
{
    /* How big is the terminal right now? (Re-checked every frame so resizing
     * the window Just Works.) Fall back to 80x24 if the ioctl is unavailable. */
    struct winsize ws;
    int cols = 80, rows = 24;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col && ws.ws_row) {
        cols = ws.ws_col;
        rows = ws.ws_row - 1;   /* leave the bottom line free of scrolling */
    }

    /* Terminal cells are about twice as tall as wide. If we mapped pixels to
     * cells 1:1 the picture would look stretched vertically, so we cap the row
     * count to preserve DOOM's 16:10 aspect for the chosen width. */
    int max_rows = (int)((double)cols * DOOMGENERIC_RESY / DOOMGENERIC_RESX * 0.5 + 0.5);
    if (rows > max_rows) rows = max_rows;
    if (cols < 1) cols = 1;
    if (rows < 1) rows = 1;

    /* Budget: up to ~24 bytes per cell (SGR colour + glyph), plus per-row
     * cursor moves, plus the home/reset preamble. */
    ensure_out((size_t)(cols + 4) * (size_t)rows * 24 + 64);

    char *p = s_out;
    /* Home the cursor (overwrite in place -- no scroll, no flicker). */
    p += sprintf(p, "\033[H");

    int last_r = -1, last_g = -1, last_b = -1;   /* skip redundant colour SGRs */

    for (int ry = 0; ry < rows; ry++) {
        /* Source pixel rows covered by this cell row. */
        int sy0 =  ry      * DOOMGENERIC_RESY / rows;
        int sy1 = (ry + 1) * DOOMGENERIC_RESY / rows;
        if (sy1 <= sy0) sy1 = sy0 + 1;

        for (int rx = 0; rx < cols; rx++) {
            int sx0 =  rx      * DOOMGENERIC_RESX / cols;
            int sx1 = (rx + 1) * DOOMGENERIC_RESX / cols;
            if (sx1 <= sx0) sx1 = sx0 + 1;

            /* Average the source block into one RGB value. */
            unsigned long sr = 0, sg = 0, sb = 0, cnt = 0;
            for (int sy = sy0; sy < sy1; sy++) {
                const pixel_t *row = &DG_ScreenBuffer[(size_t)sy * DOOMGENERIC_RESX];
                for (int sx = sx0; sx < sx1; sx++) {
                    uint32_t px = row[sx];           /* 0x00RRGGBB */
                    sr += (px >> 16) & 0xFF;
                    sg += (px >>  8) & 0xFF;
                    sb +=  px        & 0xFF;
                    cnt++;
                }
            }
            int r = (int)(sr / cnt), g = (int)(sg / cnt), b = (int)(sb / cnt);

            /* Luma -> glyph (Rec. 601 weights). */
            int luma = (r * 77 + g * 150 + b * 29) >> 8;   /* 0..255 */
            char glyph = RAMP[luma * RAMP_LEN / 255];

            /* Emit a colour escape only when the colour actually changed. */
            if (r != last_r || g != last_g || b != last_b) {
                p += sprintf(p, "\033[38;2;%d;%d;%dm", r, g, b);
                last_r = r; last_g = g; last_b = b;
            }
            *p++ = glyph;
        }
        if (ry + 1 < rows) { *p++ = '\r'; *p++ = '\n'; }
    }
    p += sprintf(p, "\033[0m");

    ssize_t w = write(STDOUT_FILENO, s_out, (size_t)(p - s_out));
    (void)w;
}

void DG_SleepMs(uint32_t ms)
{
    struct timespec ts;
    ts.tv_sec  = ms / 1000;
    ts.tv_nsec = (long)(ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
}

uint32_t DG_GetTicksMs(void)
{
    return now_ms() - s_start_ms;
}

int DG_GetKey(int *pressed, unsigned char *doomKey)
{
    /* Pull any freshly-typed bytes in, then release anything whose hold time
     * has elapsed, then hand the engine the next queued event (if any). */
    pump_terminal_input();
    flush_due_releases();

    if (s_evt_head == s_evt_tail) return 0;   /* nothing pending */

    *pressed = s_evt[s_evt_head].pressed;
    *doomKey = s_evt[s_evt_head].key;
    s_evt_head = (s_evt_head + 1) % EVT_CAP;
    return 1;
}

void DG_SetWindowTitle(const char *title)
{
    /* A terminal has no title bar we own, but it does have a window title we can
     * set with the xterm OSC 0 escape. We must NOT printf() here -- ordinary
     * stdout would scribble over the frame. */
    char esc[256];
    int n = snprintf(esc, sizeof esc, "\033]0;%s\a", title ? title : "");
    if (n > 0) {
        ssize_t r = write(STDOUT_FILENO, esc, (size_t)n);
        (void)r;
    }
}

/* ===========================================================================
 *  main -- the canonical real-port loop (see template/platform_template.c):
 *  start the engine once, then tick forever. doomgeneric_Create() honours
 *  -iwad etc., allocates DG_ScreenBuffer, and calls our DG_Init().
 * ===========================================================================*/
int main(int argc, char **argv)
{
    doomgeneric_Create(argc, argv);
    for (;;)
        doomgeneric_Tick();   /* reads input, runs logic, draws -> DG_DrawFrame */
    return 0;
}
