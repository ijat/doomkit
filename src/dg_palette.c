/* =============================================================================
 *  dg_palette.c  --  Paletted (8-bit) pixels  ->  RGB pixels.
 *  See include/genericdoom/dg_palette.h for the background on why DOOM is
 *  paletted and how the two-step (set palette, then convert) flow works.
 *  Derived from cmap_to_fb() / cmap_to_rgb565() / I_SetPalette() in i_video.c.
 *  GNU General Public License v2. See LICENSE.
 * ===========================================================================*/

#include "genericdoom/dg_palette.h"

void dg_palette_set(dg_palette_t *pal, const uint8_t rgb[768],
                    const dg_gamma_table_t gamma)
{
    int i;

    /* Walk the 768-byte source: three bytes (R, G, B) per colour, 256 colours.
     * Each channel is optionally pushed through the gamma ramp so on-screen
     * brightness matches the original game. */
    for (i = 0; i < 256; i++) {
        uint8_t r = rgb[i * 3 + 0];
        uint8_t g = rgb[i * 3 + 1];
        uint8_t b = rgb[i * 3 + 2];

        if (gamma != 0) {
            r = gamma[r];
            g = gamma[g];
            b = gamma[b];
        }

        pal->colors[i].r = r;
        pal->colors[i].g = g;
        pal->colors[i].b = b;
        pal->colors[i].a = 0;   /* DOOM has no alpha; keep the slot clear. */
    }
}

uint32_t dg_color_pack32(dg_color_t c,
                         int red_shift, int green_shift, int blue_shift)
{
    /* Slot each 8-bit channel into its requested position. With red_shift=16,
     * green_shift=8, blue_shift=0 this yields the common 0x00RRGGBB layout. */
    return ((uint32_t)c.r << red_shift)
         | ((uint32_t)c.g << green_shift)
         | ((uint32_t)c.b << blue_shift);
}

uint16_t dg_color_pack565(dg_color_t c)
{
    /* RGB565: keep the top 5 bits of red, top 6 of green, top 5 of blue, then
     * line them up as RRRRR GGGGGG BBBBB. */
    uint16_t r = (uint16_t)(c.r >> 3) << 11;
    uint16_t g = (uint16_t)(c.g >> 2) << 5;
    uint16_t b = (uint16_t)(c.b >> 3);
    return (uint16_t)(r | g | b);
}

void dg_cmap_to_fb32(uint32_t *out, const uint8_t *indices, int count,
                     const dg_palette_t *pal,
                     int red_shift, int green_shift, int blue_shift)
{
    int i;
    /* Per pixel: look the index up in the palette, pack, store. This is the
     * whole-screen hot path, so it stays a flat loop with no branches inside. */
    for (i = 0; i < count; i++) {
        out[i] = dg_color_pack32(pal->colors[indices[i]],
                                 red_shift, green_shift, blue_shift);
    }
}

void dg_cmap_to_rgb565(uint16_t *out, const uint8_t *indices, int count,
                       const dg_palette_t *pal)
{
    int i;
    for (i = 0; i < count; i++) {
        out[i] = dg_color_pack565(pal->colors[indices[i]]);
    }
}
