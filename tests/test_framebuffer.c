/* =============================================================================
 *  test_framebuffer.c  --  Unit tests for the centering/scaling layout math.
 *  Covers the "fits" and "does not fit" branches, scaling, odd-margin split,
 *  and both pixel depths. 100% coverage of src/dg_framebuffer.c.
 * ===========================================================================*/

#include "unity.h"
#include "genericdoom/dg_framebuffer.h"

void setUp(void) {}
void tearDown(void) {}

/* The canonical case: 320x200 DOOM centred in a 640x400, 32-bit buffer, 1x. */
static void test_default_640x400_32bpp(void)
{
    dg_fb_layout_t L;
    dg_fb_layout(640, 400, 320, 200, 32, 1, &L);

    TEST_ASSERT_EQUAL_INT(1, L.fits);
    TEST_ASSERT_EQUAL_INT(4, L.bytes_per_pixel);
    TEST_ASSERT_EQUAL_INT(320 * 4, L.row_stride);   /* 1280 */
    /* spare width = 320px -> 1280 bytes, split 640/640 */
    TEST_ASSERT_EQUAL_INT(640, L.x_offset);
    TEST_ASSERT_EQUAL_INT(640, L.x_offset_end);
    /* spare height = 200px -> 100 rows on top; as bytes: 100*640*4 = 256000 */
    TEST_ASSERT_EQUAL_INT(256000, L.y_offset);
}

/* Exact fit (scaling fills the buffer) -> zero margins, still fits. */
static void test_exact_fit_scaling2(void)
{
    dg_fb_layout_t L;
    dg_fb_layout(640, 400, 320, 200, 32, 2, &L);

    TEST_ASSERT_EQUAL_INT(1, L.fits);
    TEST_ASSERT_EQUAL_INT(0, L.x_offset);
    TEST_ASSERT_EQUAL_INT(0, L.x_offset_end);
    TEST_ASSERT_EQUAL_INT(0, L.y_offset);
    TEST_ASSERT_EQUAL_INT(640 * 4, L.row_stride);   /* scaled width 640 */
}

/* Framebuffer too narrow -> does not fit; offsets clamped to 0. */
static void test_does_not_fit_width(void)
{
    dg_fb_layout_t L;
    dg_fb_layout(100, 400, 320, 200, 32, 1, &L);

    TEST_ASSERT_EQUAL_INT(0, L.fits);
    TEST_ASSERT_EQUAL_INT(0, L.x_offset);
    TEST_ASSERT_EQUAL_INT(0, L.x_offset_end);
    TEST_ASSERT_EQUAL_INT(0, L.y_offset);
    /* row_stride is still reported (callers may want it for diagnostics). */
    TEST_ASSERT_EQUAL_INT(320 * 4, L.row_stride);
}

/* Framebuffer too short -> does not fit (covers the height side of the OR). */
static void test_does_not_fit_height(void)
{
    dg_fb_layout_t L;
    dg_fb_layout(640, 100, 320, 200, 32, 1, &L);

    TEST_ASSERT_EQUAL_INT(0, L.fits);
    TEST_ASSERT_EQUAL_INT(0, L.y_offset);
}

/* Odd leftover byte goes to the RIGHT margin (x_offset_end), matching the
 * original engine's integer-halving behaviour. */
static void test_odd_margin_split(void)
{
    dg_fb_layout_t L;
    /* 1 byte per pixel, 1px of spare width -> 1 spare byte. */
    dg_fb_layout(321, 200, 320, 200, 8, 1, &L);

    TEST_ASSERT_EQUAL_INT(1, L.fits);
    TEST_ASSERT_EQUAL_INT(1, L.bytes_per_pixel);
    TEST_ASSERT_EQUAL_INT(0, L.x_offset);      /* 1 / 2 == 0 */
    TEST_ASSERT_EQUAL_INT(1, L.x_offset_end);  /* remainder to the right */
}

/* 16-bit depth path (bytes_per_pixel = 2). */
static void test_16bpp(void)
{
    dg_fb_layout_t L;
    dg_fb_layout(640, 400, 320, 200, 16, 1, &L);

    TEST_ASSERT_EQUAL_INT(1, L.fits);
    TEST_ASSERT_EQUAL_INT(2, L.bytes_per_pixel);
    TEST_ASSERT_EQUAL_INT(320 * 2, L.row_stride);
    TEST_ASSERT_EQUAL_INT(320 * 2 / 2 * 1, L.x_offset); /* spare 320px*2 /2 = 320 */
    TEST_ASSERT_EQUAL_INT(320, L.x_offset);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_default_640x400_32bpp);
    RUN_TEST(test_exact_fit_scaling2);
    RUN_TEST(test_does_not_fit_width);
    RUN_TEST(test_does_not_fit_height);
    RUN_TEST(test_odd_margin_split);
    RUN_TEST(test_16bpp);
    return UNITY_END();
}
