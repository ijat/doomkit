/* =============================================================================
 *  dg_keys.h  --  DOOM key codes.
 * -----------------------------------------------------------------------------
 *  Copyright (C) 1993-1996 id Software, Inc.
 *  Copyright (C) 2005-2014 Simon Howard.
 *  Cleaned/commented for genericdoom-cleancode.
 *  GNU General Public License v2. See LICENSE.
 * =============================================================================
 *
 *  These are the key codes the DOOM engine understands. When you report a key
 *  through DG_GetKey(), the `doomKey` byte you hand back must be one of these
 *  values -- not your platform's raw scancode. The translation from a host key
 *  to one of these is exactly what dg_keymap.h does for you.
 *
 *  THE ENCODING (why the numbers look odd)
 *  ---------------------------------------
 *  DOOM packs key codes into a single byte and reuses the printable-ASCII range
 *  for ordinary keys, so the letter 'a' is literally 0x61 and '5' is 0x35.
 *  Non-printable keys are given values *outside* the printable range so they
 *  can't collide with a real character:
 *    - Editing keys (Enter, Tab, Backspace, Escape) use their classic ASCII
 *      control codes (13, 9, 0x7f, 27).
 *    - Movement / action keys live in the 0xA0-0xAF block.
 *    - "Extended" keys (function keys, modifiers, navigation cluster) are the
 *      original PC scancode + 0x80, which is why you see the (0x80+...) form.
 *
 *  You normally don't need to memorise any of this -- just pass these macros
 *  around by name.
 * ===========================================================================*/

#ifndef GENERICDOOM_DG_KEYS_H
#define GENERICDOOM_DG_KEYS_H

/* --- Movement and the two core actions ----------------------------------- */
#define KEY_RIGHTARROW  0xae
#define KEY_LEFTARROW   0xac
#define KEY_UPARROW     0xad
#define KEY_DOWNARROW   0xaf
#define KEY_STRAFE_L    0xa0   /* strafe left  */
#define KEY_STRAFE_R    0xa1   /* strafe right */
#define KEY_USE         0xa2   /* open doors / flip switches (default: Space) */
#define KEY_FIRE        0xa3   /* shoot (default: Ctrl) */

/* --- Editing / control keys (classic ASCII control codes) ----------------- */
#define KEY_ESCAPE      27
#define KEY_ENTER       13
#define KEY_TAB         9
#define KEY_BACKSPACE   0x7f
#define KEY_PAUSE       0xff

/* --- Punctuation used by the menus --------------------------------------- */
#define KEY_EQUALS      0x3d   /* '=' / '+' : enlarge screen */
#define KEY_MINUS       0x2d   /* '-'       : shrink screen  */

/* --- Function keys (scancode + 0x80) -------------------------------------- */
#define KEY_F1          (0x80+0x3b)
#define KEY_F2          (0x80+0x3c)
#define KEY_F3          (0x80+0x3d)
#define KEY_F4          (0x80+0x3e)
#define KEY_F5          (0x80+0x3f)
#define KEY_F6          (0x80+0x40)
#define KEY_F7          (0x80+0x41)
#define KEY_F8          (0x80+0x42)
#define KEY_F9          (0x80+0x43)
#define KEY_F10         (0x80+0x44)
#define KEY_F11         (0x80+0x57)
#define KEY_F12         (0x80+0x58)

/* --- Modifier keys -------------------------------------------------------- */
#define KEY_RSHIFT      (0x80+0x36)   /* run modifier */
#define KEY_RCTRL       (0x80+0x1d)   /* fire modifier */
#define KEY_RALT        (0x80+0x38)   /* strafe modifier */
#define KEY_LALT        KEY_RALT      /* DOOM treats both Alt keys the same */

/* --- Lock keys and Print Screen ------------------------------------------ */
#define KEY_CAPSLOCK    (0x80+0x3a)
#define KEY_NUMLOCK     (0x80+0x45)
#define KEY_SCRLCK      (0x80+0x46)
#define KEY_PRTSCR      (0x80+0x59)

/* --- Navigation cluster --------------------------------------------------- */
#define KEY_HOME        (0x80+0x47)
#define KEY_END         (0x80+0x4f)
#define KEY_PGUP        (0x80+0x49)
#define KEY_PGDN        (0x80+0x51)
#define KEY_INS         (0x80+0x52)
#define KEY_DEL         (0x80+0x53)

/* --- Numeric keypad ------------------------------------------------------- *
 *  Many keypad keys deliberately alias the navigation/arrow keys, matching
 *  the way the original PC keyboard behaved with Num Lock off.
 * -------------------------------------------------------------------------- */
#define KEYP_0          0
#define KEYP_1          KEY_END
#define KEYP_2          KEY_DOWNARROW
#define KEYP_3          KEY_PGDN
#define KEYP_4          KEY_LEFTARROW
#define KEYP_5          '5'
#define KEYP_6          KEY_RIGHTARROW
#define KEYP_7          KEY_HOME
#define KEYP_8          KEY_UPARROW
#define KEYP_9          KEY_PGUP

#define KEYP_DIVIDE     '/'
#define KEYP_PLUS       '+'
#define KEYP_MINUS      '-'
#define KEYP_MULTIPLY   '*'
#define KEYP_PERIOD     0
#define KEYP_EQUALS     KEY_EQUALS
#define KEYP_ENTER      KEY_ENTER

#endif /* GENERICDOOM_DG_KEYS_H */
