/* =============================================================================
 *  dg_keymap.h  --  Translate host keyboard codes into DOOM key codes.
 * -----------------------------------------------------------------------------
 *  Derived from convertToDoomKey() in doomgeneric's SDL port.
 *  GNU General Public License v2. See LICENSE.
 * =============================================================================
 *
 *  WHY YOU NEED THIS
 *  -----------------
 *  Every windowing system has its own numbers for keys. SDL calls the up arrow
 *  SDLK_UP; X11 calls it XK_Up; Windows calls it VK_UP. The DOOM engine, of
 *  course, knows none of these -- it only understands the codes in dg_keys.h.
 *  So somewhere in every port a translation has to happen.
 *
 *  This module gives you a ready-made translation for SDL-style key symbols
 *  (which also happen to be ASCII for printable keys, so the same function is a
 *  great starting point for most platforms). If your platform uses different
 *  numbers, copy this function and swap the case labels for your own constants;
 *  the shape stays identical.
 *
 *  THE DEFAULT RULE
 *  ----------------
 *  Anything not handled by an explicit case is assumed to already be ASCII and
 *  is lower-cased (DOOM expects lower-case letters). That single fallback
 *  covers every letter, digit and most punctuation for free, which is why the
 *  explicit list below only needs the "special" keys.
 * ===========================================================================*/

#ifndef DOOMKIT_DG_KEYMAP_H
#define DOOMKIT_DG_KEYMAP_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  Translate an SDL-style key symbol into a DOOM key code (see dg_keys.h).
 *
 *  Returns the DOOM key code as an unsigned char. Unknown / printable keys are
 *  passed through lower-cased. This function is pure: it has no side effects and
 *  always returns the same output for the same input, which is what makes it
 *  trivially unit-testable.
 *
 *  The `key` argument is taken as `unsigned int` to match the width of typical
 *  key-symbol enums; SDLK_* values fit comfortably.
 */
unsigned char dg_keymap_from_sdl(unsigned int key);

#ifdef __cplusplus
}
#endif

#endif /* DOOMKIT_DG_KEYMAP_H */
