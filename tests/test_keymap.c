/* =============================================================================
 *  test_keymap.c  --  Unit tests for host-key -> DOOM-key translation.
 *  Exercises every case label plus both default (lower-case / pass-through)
 *  paths, for 100% coverage of src/dg_keymap.c.
 *
 *  The SDL key-symbol numbers are restated here (matching src/dg_keymap.c) so
 *  the test needs no SDL headers.
 * ===========================================================================*/

#include "unity.h"
#include "doomkit/dg_keymap.h"
#include "doomkit/dg_keys.h"

void setUp(void) {}
void tearDown(void) {}

#define BIT 0x40000000u

static void test_editing_keys(void)
{
    TEST_ASSERT_EQUAL_UINT8(KEY_ENTER,  dg_keymap_from_sdl(13)); /* RETURN */
    TEST_ASSERT_EQUAL_UINT8(KEY_ESCAPE, dg_keymap_from_sdl(27)); /* ESCAPE */
}

static void test_arrows(void)
{
    TEST_ASSERT_EQUAL_UINT8(KEY_LEFTARROW,  dg_keymap_from_sdl(BIT | 0x50));
    TEST_ASSERT_EQUAL_UINT8(KEY_RIGHTARROW, dg_keymap_from_sdl(BIT | 0x4F));
    TEST_ASSERT_EQUAL_UINT8(KEY_UPARROW,    dg_keymap_from_sdl(BIT | 0x52));
    TEST_ASSERT_EQUAL_UINT8(KEY_DOWNARROW,  dg_keymap_from_sdl(BIT | 0x51));
}

static void test_actions_and_modifiers(void)
{
    /* Both ctrl keys fold to FIRE. */
    TEST_ASSERT_EQUAL_UINT8(KEY_FIRE, dg_keymap_from_sdl(BIT | 0xE0)); /* LCTRL */
    TEST_ASSERT_EQUAL_UINT8(KEY_FIRE, dg_keymap_from_sdl(BIT | 0xE4)); /* RCTRL */

    TEST_ASSERT_EQUAL_UINT8(KEY_USE, dg_keymap_from_sdl(32));          /* SPACE */

    TEST_ASSERT_EQUAL_UINT8(KEY_RSHIFT, dg_keymap_from_sdl(BIT | 0xE1)); /* LSHIFT */
    TEST_ASSERT_EQUAL_UINT8(KEY_RSHIFT, dg_keymap_from_sdl(BIT | 0xE5)); /* RSHIFT */

    TEST_ASSERT_EQUAL_UINT8(KEY_LALT, dg_keymap_from_sdl(BIT | 0xE2));   /* LALT */
    TEST_ASSERT_EQUAL_UINT8(KEY_LALT, dg_keymap_from_sdl(BIT | 0xE6));   /* RALT */
}

static void test_function_keys(void)
{
    TEST_ASSERT_EQUAL_UINT8(KEY_F2,  dg_keymap_from_sdl(BIT | 0x3B));
    TEST_ASSERT_EQUAL_UINT8(KEY_F3,  dg_keymap_from_sdl(BIT | 0x3C));
    TEST_ASSERT_EQUAL_UINT8(KEY_F4,  dg_keymap_from_sdl(BIT | 0x3D));
    TEST_ASSERT_EQUAL_UINT8(KEY_F5,  dg_keymap_from_sdl(BIT | 0x3E));
    TEST_ASSERT_EQUAL_UINT8(KEY_F6,  dg_keymap_from_sdl(BIT | 0x3F));
    TEST_ASSERT_EQUAL_UINT8(KEY_F7,  dg_keymap_from_sdl(BIT | 0x40));
    TEST_ASSERT_EQUAL_UINT8(KEY_F8,  dg_keymap_from_sdl(BIT | 0x41));
    TEST_ASSERT_EQUAL_UINT8(KEY_F9,  dg_keymap_from_sdl(BIT | 0x42));
    TEST_ASSERT_EQUAL_UINT8(KEY_F10, dg_keymap_from_sdl(BIT | 0x43));
    TEST_ASSERT_EQUAL_UINT8(KEY_F11, dg_keymap_from_sdl(BIT | 0x44));
}

static void test_screen_size_keys(void)
{
    TEST_ASSERT_EQUAL_UINT8(KEY_EQUALS, dg_keymap_from_sdl(61)); /* '=' */
    TEST_ASSERT_EQUAL_UINT8(KEY_EQUALS, dg_keymap_from_sdl(43)); /* '+' */
    TEST_ASSERT_EQUAL_UINT8(KEY_MINUS,  dg_keymap_from_sdl(45)); /* '-' */
}

static void test_default_path(void)
{
    /* Uppercase letters are lower-cased (the `A`..`Z` branch). */
    TEST_ASSERT_EQUAL_UINT8('a', dg_keymap_from_sdl('A'));
    TEST_ASSERT_EQUAL_UINT8('z', dg_keymap_from_sdl('Z'));

    /* Already-lowercase letters pass straight through (the else branch). */
    TEST_ASSERT_EQUAL_UINT8('q', dg_keymap_from_sdl('q'));

    /* Digits and punctuation are not letters, so pass through unchanged. */
    TEST_ASSERT_EQUAL_UINT8('5', dg_keymap_from_sdl('5'));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_editing_keys);
    RUN_TEST(test_arrows);
    RUN_TEST(test_actions_and_modifiers);
    RUN_TEST(test_function_keys);
    RUN_TEST(test_screen_size_keys);
    RUN_TEST(test_default_path);
    return UNITY_END();
}
