/* =============================================================================
 *  dg_keymap.c  --  SDL key symbol  ->  DOOM key code.
 *  See include/genericdoom/dg_keymap.h for the rationale.
 *  Derived from convertToDoomKey() in doomgeneric_sdl.c.
 *  GNU General Public License v2. See LICENSE.
 * ===========================================================================*/

#include "genericdoom/dg_keymap.h"
#include "genericdoom/dg_keys.h"

/* -----------------------------------------------------------------------------
 *  We deliberately do NOT #include <SDL.h> here. Depending on SDL would mean
 *  you could not build or unit-test this translation without installing SDL,
 *  which defeats the purpose of a clean, portable reference. Instead we restate
 *  the handful of SDL key-symbol values we care about. These are SDL2's real
 *  constants (printable keys equal their ASCII code; special keys carry SDL's
 *  0x40000000 "scancode" bit). If you actually build the SDL example, you can
 *  pass SDLK_* values straight in -- they are these same numbers.
 * ---------------------------------------------------------------------------*/
#define SDLK_RETURN    13       /* '\r' */
#define SDLK_ESCAPE    27       /* '\033' */
#define SDLK_SPACE     32       /* ' ' */
#define SDLK_EQUALS    61       /* '=' */
#define SDLK_PLUS      43       /* '+' (only via shift on some layouts) */
#define SDLK_MINUS     45       /* '-' */

#define SDL_SCANCODE_BIT 0x40000000u  /* SDL marks non-ASCII keys with this */

#define SDLK_LEFT      (SDL_SCANCODE_BIT | 0x50)
#define SDLK_RIGHT     (SDL_SCANCODE_BIT | 0x4F)
#define SDLK_UP        (SDL_SCANCODE_BIT | 0x52)
#define SDLK_DOWN      (SDL_SCANCODE_BIT | 0x51)

#define SDLK_LCTRL     (SDL_SCANCODE_BIT | 0xE0)
#define SDLK_RCTRL     (SDL_SCANCODE_BIT | 0xE4)
#define SDLK_LSHIFT    (SDL_SCANCODE_BIT | 0xE1)
#define SDLK_RSHIFT    (SDL_SCANCODE_BIT | 0xE5)
#define SDLK_LALT      (SDL_SCANCODE_BIT | 0xE2)
#define SDLK_RALT      (SDL_SCANCODE_BIT | 0xE6)

/* Function keys are the SDL scancode (F1==0x3A) OR'd with the scancode bit. */
#define SDLK_F2        (SDL_SCANCODE_BIT | 0x3B)
#define SDLK_F3        (SDL_SCANCODE_BIT | 0x3C)
#define SDLK_F4        (SDL_SCANCODE_BIT | 0x3D)
#define SDLK_F5        (SDL_SCANCODE_BIT | 0x3E)
#define SDLK_F6        (SDL_SCANCODE_BIT | 0x3F)
#define SDLK_F7        (SDL_SCANCODE_BIT | 0x40)
#define SDLK_F8        (SDL_SCANCODE_BIT | 0x41)
#define SDLK_F9        (SDL_SCANCODE_BIT | 0x42)
#define SDLK_F10       (SDL_SCANCODE_BIT | 0x43)
#define SDLK_F11       (SDL_SCANCODE_BIT | 0x44)

/* Well-defined ASCII lower-casing. The original used tolower(); we spell it out
 * so the result is identical for letters and never depends on the C locale. */
static unsigned char ascii_tolower(unsigned int key)
{
    if (key >= 'A' && key <= 'Z') {
        return (unsigned char)(key + ('a' - 'A'));
    }
    return (unsigned char)key;
}

unsigned char dg_keymap_from_sdl(unsigned int key)
{
    switch (key) {
        /* Editing keys */
        case SDLK_RETURN:  return KEY_ENTER;
        case SDLK_ESCAPE:  return KEY_ESCAPE;

        /* Arrow keys */
        case SDLK_LEFT:    return KEY_LEFTARROW;
        case SDLK_RIGHT:   return KEY_RIGHTARROW;
        case SDLK_UP:      return KEY_UPARROW;
        case SDLK_DOWN:    return KEY_DOWNARROW;

        /* Actions and modifiers. Both left/right variants fold onto the single
         * code DOOM understands. */
        case SDLK_LCTRL:
        case SDLK_RCTRL:   return KEY_FIRE;
        case SDLK_SPACE:   return KEY_USE;
        case SDLK_LSHIFT:
        case SDLK_RSHIFT:  return KEY_RSHIFT;
        case SDLK_LALT:
        case SDLK_RALT:    return KEY_LALT;

        /* Function keys (the subset DOOM's menus use) */
        case SDLK_F2:      return KEY_F2;
        case SDLK_F3:      return KEY_F3;
        case SDLK_F4:      return KEY_F4;
        case SDLK_F5:      return KEY_F5;
        case SDLK_F6:      return KEY_F6;
        case SDLK_F7:      return KEY_F7;
        case SDLK_F8:      return KEY_F8;
        case SDLK_F9:      return KEY_F9;
        case SDLK_F10:     return KEY_F10;
        case SDLK_F11:     return KEY_F11;

        /* Screen size controls */
        case SDLK_EQUALS:
        case SDLK_PLUS:    return KEY_EQUALS;
        case SDLK_MINUS:   return KEY_MINUS;

        /* Everything else is already ASCII; hand it back lower-cased. */
        default:           return ascii_tolower(key);
    }
}
