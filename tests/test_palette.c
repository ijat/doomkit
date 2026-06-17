/* =============================================================================
 *  test_palette.c  --  Unit tests for paletted -> RGB conversion.
 *  Covers: palette load with and without gamma, the 32-bit and RGB565 packers,
 *  and both bulk converters. 100% coverage of src/dg_palette.c.
 * ===========================================================================*/

#include "unity.h"
#include "genericdoom/dg_palette.h"

void setUp(void) {}
void tearDown(void) {}

/* Build a deterministic 768-byte playpal: colour i = (r=i, g=255-i, b=i/2). */
static void make_test_rgb(uint8_t rgb[768])
{
    for (int i = 0; i < 256; i++) {
        rgb[i * 3 + 0] = (uint8_t)i;
        rgb[i * 3 + 1] = (uint8_t)(255 - i);
        rgb[i * 3 + 2] = (uint8_t)(i / 2);
    }
}

/* An inverting gamma ramp, so we can prove the ramp was actually applied. */
static uint8_t g_invert[256];
static void make_invert_gamma(void)
{
    for (int i = 0; i < 256; i++) g_invert[i] = (uint8_t)(255 - i);
}

/* Without a gamma ramp (NULL), channels are stored verbatim. */
static void test_palette_set_no_gamma(void)
{
    uint8_t rgb[768];
    make_test_rgb(rgb);

    dg_palette_t pal;
    dg_palette_set(&pal, rgb, 0 /* NULL gamma */);

    TEST_ASSERT_EQUAL_UINT8(0,   pal.colors[0].r);
    TEST_ASSERT_EQUAL_UINT8(255, pal.colors[0].g);
    TEST_ASSERT_EQUAL_UINT8(0,   pal.colors[0].b);
    TEST_ASSERT_EQUAL_UINT8(0,   pal.colors[0].a);

    TEST_ASSERT_EQUAL_UINT8(100, pal.colors[100].r);
    TEST_ASSERT_EQUAL_UINT8(155, pal.colors[100].g);
    TEST_ASSERT_EQUAL_UINT8(50,  pal.colors[100].b);
}

/* With a gamma ramp, every channel is pushed through it. */
static void test_palette_set_with_gamma(void)
{
    uint8_t rgb[768];
    make_test_rgb(rgb);
    make_invert_gamma();

    dg_palette_t pal;
    dg_palette_set(&pal, rgb, g_invert);

    /* colour 100: raw (100,155,50) -> inverted (155,100,205) */
    TEST_ASSERT_EQUAL_UINT8(155, pal.colors[100].r);
    TEST_ASSERT_EQUAL_UINT8(100, pal.colors[100].g);
    TEST_ASSERT_EQUAL_UINT8(205, pal.colors[100].b);
}

static void test_pack32_layouts(void)
{
    dg_color_t c = { 0x12, 0x34, 0x56, 0x00 };

    /* Standard 0x00RRGGBB. */
    TEST_ASSERT_EQUAL_HEX32(0x00123456u, dg_color_pack32(c, 16, 8, 0));
    /* A different layout (BGR) just moves the shifts. */
    TEST_ASSERT_EQUAL_HEX32(0x00563412u, dg_color_pack32(c, 0, 8, 16));
}

static void test_pack565(void)
{
    dg_color_t white = { 0xFF, 0xFF, 0xFF, 0 };
    dg_color_t black = { 0x00, 0x00, 0x00, 0 };
    dg_color_t red   = { 0xFF, 0x00, 0x00, 0 };
    dg_color_t green = { 0x00, 0xFF, 0x00, 0 };
    dg_color_t blue  = { 0x00, 0x00, 0xFF, 0 };

    TEST_ASSERT_EQUAL_HEX16(0xFFFF, dg_color_pack565(white));
    TEST_ASSERT_EQUAL_HEX16(0x0000, dg_color_pack565(black));
    TEST_ASSERT_EQUAL_HEX16(0xF800, dg_color_pack565(red));
    TEST_ASSERT_EQUAL_HEX16(0x07E0, dg_color_pack565(green));
    TEST_ASSERT_EQUAL_HEX16(0x001F, dg_color_pack565(blue));
}

static void test_cmap_to_fb32(void)
{
    uint8_t rgb[768];
    make_test_rgb(rgb);
    dg_palette_t pal;
    dg_palette_set(&pal, rgb, 0);

    uint8_t indices[4] = { 0, 1, 100, 255 };
    uint32_t out[4] = { 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD };

    dg_cmap_to_fb32(out, indices, 4, &pal, 16, 8, 0);

    /* index 0   -> (0,255,0)     */
    TEST_ASSERT_EQUAL_HEX32(0x0000FF00u, out[0]);
    /* index 100 -> (100,155,50)  */
    TEST_ASSERT_EQUAL_HEX32(0x00649B32u, out[2]);
    /* index 255 -> (255,0,127)   */
    TEST_ASSERT_EQUAL_HEX32(0x00FF007Fu, out[3]);
}

static void test_cmap_to_rgb565(void)
{
    uint8_t rgb[768];
    make_test_rgb(rgb);
    dg_palette_t pal;
    dg_palette_set(&pal, rgb, 0);

    uint8_t indices[2] = { 0, 255 };
    uint16_t out[2] = { 0xAAAA, 0xAAAA };

    dg_cmap_to_rgb565(out, indices, 2, &pal);

    /* index 0 -> (0,255,0) -> green565 0x07E0 */
    TEST_ASSERT_EQUAL_HEX16(0x07E0, out[0]);
    /* index 255 -> (255,0,127): r=0xFF>>3<<11=0xF800, b=0x7F>>3=0x0F */
    TEST_ASSERT_EQUAL_HEX16((uint16_t)(0xF800 | 0x000F), out[1]);
}

/* A zero-length conversion must be a no-op (loop runs zero times). */
static void test_cmap_zero_count(void)
{
    dg_palette_t pal;
    uint8_t rgb[768] = {0};
    dg_palette_set(&pal, rgb, 0);

    uint32_t out32 = 0x1234;
    uint16_t out16 = 0x5678;
    dg_cmap_to_fb32(&out32, (const uint8_t *)"", 0, &pal, 16, 8, 0);
    dg_cmap_to_rgb565(&out16, (const uint8_t *)"", 0, &pal);

    TEST_ASSERT_EQUAL_HEX32(0x1234, out32);
    TEST_ASSERT_EQUAL_HEX16(0x5678, out16);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_palette_set_no_gamma);
    RUN_TEST(test_palette_set_with_gamma);
    RUN_TEST(test_pack32_layouts);
    RUN_TEST(test_pack565);
    RUN_TEST(test_cmap_to_fb32);
    RUN_TEST(test_cmap_to_rgb565);
    RUN_TEST(test_cmap_zero_count);
    return UNITY_END();
}
