/* =============================================================================
 *  test_keyqueue.c  --  Unit tests for the key event ring buffer.
 *  Aims for 100% line + branch coverage of src/dg_keyqueue.c.
 * ===========================================================================*/

#include "unity.h"
#include "doomkit/dg_keyqueue.h"
#include "doomkit/dg_keys.h"

/* Unity calls these around every test; we need no shared fixture. */
void setUp(void) {}
void tearDown(void) {}

/* A freshly initialised queue reports empty, not full, count 0. */
static void test_init_is_empty(void)
{
    dg_keyqueue_t q;
    dg_keyqueue_init(&q);

    TEST_ASSERT_TRUE(dg_keyqueue_is_empty(&q));
    TEST_ASSERT_FALSE(dg_keyqueue_is_full(&q));
    TEST_ASSERT_EQUAL_UINT(0, dg_keyqueue_count(&q));
}

/* Popping an empty queue returns 0 and touches nothing. */
static void test_pop_empty_returns_zero(void)
{
    dg_keyqueue_t q;
    dg_keyqueue_init(&q);

    int pressed = 99;
    unsigned char key = 0xAB;
    TEST_ASSERT_EQUAL_INT(0, dg_keyqueue_pop(&q, &pressed, &key));
    /* Out-params left untouched on an empty pop. */
    TEST_ASSERT_EQUAL_INT(99, pressed);
    TEST_ASSERT_EQUAL_UINT8(0xAB, key);
}

/* A single push then pop round-trips the data in FIFO order. */
static void test_push_then_pop_roundtrip(void)
{
    dg_keyqueue_t q;
    dg_keyqueue_init(&q);

    TEST_ASSERT_EQUAL_INT(1, dg_keyqueue_push(&q, 1, KEY_FIRE));
    TEST_ASSERT_FALSE(dg_keyqueue_is_empty(&q));
    TEST_ASSERT_EQUAL_UINT(1, dg_keyqueue_count(&q));

    int pressed = 0;
    unsigned char key = 0;
    TEST_ASSERT_EQUAL_INT(1, dg_keyqueue_pop(&q, &pressed, &key));
    TEST_ASSERT_EQUAL_INT(1, pressed);
    TEST_ASSERT_EQUAL_UINT8(KEY_FIRE, key);
    TEST_ASSERT_TRUE(dg_keyqueue_is_empty(&q));
}

/* `pressed` is normalised to exactly 1 or 0 regardless of the input value. */
static void test_pressed_is_normalised(void)
{
    dg_keyqueue_t q;
    dg_keyqueue_init(&q);

    dg_keyqueue_push(&q, 5, KEY_USE);    /* any non-zero -> 1 */
    dg_keyqueue_push(&q, 0, KEY_USE);    /* zero        -> 0 */

    int pressed; unsigned char key;
    dg_keyqueue_pop(&q, &pressed, &key);
    TEST_ASSERT_EQUAL_INT(1, pressed);
    dg_keyqueue_pop(&q, &pressed, &key);
    TEST_ASSERT_EQUAL_INT(0, pressed);
}

/* FIFO ordering is preserved across several events. */
static void test_fifo_order(void)
{
    dg_keyqueue_t q;
    dg_keyqueue_init(&q);

    for (unsigned char k = 1; k <= 3; k++) {
        dg_keyqueue_push(&q, 1, k);
    }
    TEST_ASSERT_EQUAL_UINT(3, dg_keyqueue_count(&q));

    for (unsigned char expected = 1; expected <= 3; expected++) {
        int pressed; unsigned char key;
        TEST_ASSERT_EQUAL_INT(1, dg_keyqueue_pop(&q, &pressed, &key));
        TEST_ASSERT_EQUAL_UINT8(expected, key);
    }
    TEST_ASSERT_TRUE(dg_keyqueue_is_empty(&q));
}

/* Filling to capacity makes is_full true; the next push is dropped (returns 0)
 * and does not corrupt the data already queued. */
static void test_fill_and_overflow(void)
{
    dg_keyqueue_t q;
    dg_keyqueue_init(&q);

    /* Usable capacity is one less than the array size. */
    unsigned int capacity = DG_KEYQUEUE_SIZE - 1;
    for (unsigned int i = 0; i < capacity; i++) {
        TEST_ASSERT_EQUAL_INT(1, dg_keyqueue_push(&q, 1, (unsigned char)(i + 1)));
    }

    TEST_ASSERT_TRUE(dg_keyqueue_is_full(&q));
    TEST_ASSERT_EQUAL_UINT(capacity, dg_keyqueue_count(&q));

    /* Overflowing push is rejected. */
    TEST_ASSERT_EQUAL_INT(0, dg_keyqueue_push(&q, 1, 0xEE));

    /* The original contents survive intact and in order. */
    for (unsigned int i = 0; i < capacity; i++) {
        int pressed; unsigned char key;
        dg_keyqueue_pop(&q, &pressed, &key);
        TEST_ASSERT_EQUAL_UINT8((unsigned char)(i + 1), key);
    }
}

/* Interleaving pushes and pops drives the indices past the wrap point, proving
 * the modulo arithmetic (and count across a wrap) is correct. */
static void test_wraparound(void)
{
    dg_keyqueue_t q;
    dg_keyqueue_init(&q);

    int pressed; unsigned char key;
    /* Three full laps around the ring, one in / one out each step. */
    for (unsigned int i = 0; i < DG_KEYQUEUE_SIZE * 3; i++) {
        unsigned char k = (unsigned char)(i & 0x7F);
        TEST_ASSERT_EQUAL_INT(1, dg_keyqueue_push(&q, 1, k));
        TEST_ASSERT_EQUAL_UINT(1, dg_keyqueue_count(&q));
        TEST_ASSERT_EQUAL_INT(1, dg_keyqueue_pop(&q, &pressed, &key));
        TEST_ASSERT_EQUAL_UINT8(k, key);
        TEST_ASSERT_TRUE(dg_keyqueue_is_empty(&q));
    }
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_init_is_empty);
    RUN_TEST(test_pop_empty_returns_zero);
    RUN_TEST(test_push_then_pop_roundtrip);
    RUN_TEST(test_pressed_is_normalised);
    RUN_TEST(test_fifo_order);
    RUN_TEST(test_fill_and_overflow);
    RUN_TEST(test_wraparound);
    return UNITY_END();
}
