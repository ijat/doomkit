/* =============================================================================
 *  minimal_main.c  --  The smallest possible host program.
 * -----------------------------------------------------------------------------
 *  GNU General Public License v2. See LICENSE.
 * =============================================================================
 *
 *  This is the entire "main loop" of a genericdoom port. It does just two
 *  things: start the engine once, then tick it forever. The engine pulls input,
 *  runs the game, renders into DG_ScreenBuffer and calls your DG_DrawFrame()
 *  inside each tick -- so this loop stays this small no matter the platform.
 *
 *  To build a real program you compile this together with:
 *    - your platform file implementing the six DG_* callbacks, and
 *    - the DOOM engine sources (which provide doomgeneric_Create/_Tick).
 *
 *  Note: many real ports put main() inside their platform file instead (the SDL
 *  example does), because some platforms need to own the loop -- emscripten, for
 *  instance, hands the loop to the browser. Use whichever fits your platform.
 * ===========================================================================*/

#include "genericdoom/genericdoom.h"

int main(int argc, char **argv)
{
    /* One-time startup: parses argv (e.g. -iwad doom1.wad), allocates the
     * screen buffer, calls your DG_Init(), and boots DOOM. */
    doomgeneric_Create(argc, argv);

    /* Advance one frame per iteration, forever. Each call ends by presenting a
     * frame through your DG_DrawFrame(). */
    for (;;) {
        doomgeneric_Tick();
    }

    return 0;
}
