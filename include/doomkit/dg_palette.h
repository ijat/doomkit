/* =============================================================================
 *  dg_palette.h  --  Turn DOOM's 8-bit paletted pixels into real RGB pixels.
 * -----------------------------------------------------------------------------
 *  Derived from cmap_to_fb() / cmap_to_rgb565() / I_SetPalette() in i_video.c.
 *  GNU General Public License v2. See LICENSE.
 * =============================================================================
 *
 *  BACKGROUND: WHY DOOM IS PALETTED
 *  --------------------------------
 *  DOOM is from 1993, when video memory was scarce. Instead of storing a full
 *  colour per pixel, every pixel is a single byte: an *index* (0-255) into a
 *  256-entry colour table called the palette. The artwork in the WAD is all
 *  stored this way. To show it on a modern truecolour display you must look up
 *  each index in the palette and expand it into real red/green/blue.
 *
 *  This module does that lookup. It is split into two steps that mirror how the
 *  engine works:
 *
 *    1. dg_palette_set()  -- install the active 256-colour palette. DOOM swaps
 *       palettes constantly for effects: the screen flashes red when you take
 *       damage, gold when you grab an item, etc. Each flash is a different
 *       palette handed to this function.
 *
 *    2. dg_cmap_to_*()    -- convert a row/buffer of indices into output pixels
 *       using the palette currently installed.
 *
 *  GAMMA
 *  -----
 *  DOOM ships several gamma-correction tables (the F11 key cycles them). When
 *  you install a palette you pass which gamma table to apply; the channel
 *  values are pushed through it so brightness matches the original game.
 *
 *  PURITY
 *  ------
 *  Everything here works on an explicit dg_palette_t you own -- there is no
 *  hidden global state. That is a deliberate change from the original engine
 *  (which used file-scope globals) and is what makes these functions easy to
 *  unit-test.
 * ===========================================================================*/

#ifndef DOOMKIT_DG_PALETTE_H
#define DOOMKIT_DG_PALETTE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* One resolved colour. `a` is kept so the struct maps cleanly onto a 32-bit
 * pixel; DOOM itself does not use alpha. */
typedef struct {
    uint8_t r, g, b, a;
} dg_color_t;

/* A fully resolved 256-entry palette, ready for fast index lookups. */
typedef struct {
    dg_color_t colors[256];
} dg_palette_t;

/* A 256-entry gamma ramp: gamma[input] -> corrected output. Pass NULL to
 * dg_palette_set() for the identity ramp (no correction). */
typedef const uint8_t dg_gamma_table_t[256];

/*
 *  Install a palette.
 *    pal       -- destination palette to fill (you own it)
 *    rgb       -- 768 bytes: 256 colours x {R,G,B}, exactly DOOM's playpal
 *                 lump format
 *    gamma     -- a 256-entry correction ramp, or NULL for no correction
 *
 *  After this call `pal` is ready to use with the conversion functions below.
 */
void dg_palette_set(dg_palette_t *pal, const uint8_t rgb[768],
                    const dg_gamma_table_t gamma);

/*
 *  Convert `count` palette indices into 32-bit pixels.
 *    out          -- destination, `count` uint32_t pixels
 *    indices      -- source, `count` bytes
 *    The channels are placed at the given bit offsets, so you can match any
 *    32-bit layout your display wants. For the usual 0x00RRGGBB pass
 *    red_shift=16, green_shift=8, blue_shift=0.
 *
 *  This is the hot path that runs once per frame on the whole screen, so it is
 *  kept deliberately simple and branch-free per pixel.
 */
void dg_cmap_to_fb32(uint32_t *out, const uint8_t *indices, int count,
                     const dg_palette_t *pal,
                     int red_shift, int green_shift, int blue_shift);

/*
 *  Convert `count` palette indices into 16-bit RGB565 pixels (5 bits red,
 *  6 green, 5 blue) -- the format many embedded LCDs use. Same idea as the
 *  32-bit version, just narrower.
 */
void dg_cmap_to_rgb565(uint16_t *out, const uint8_t *indices, int count,
                       const dg_palette_t *pal);

/*
 *  Pack one resolved colour into a 32-bit pixel at the given channel offsets.
 *  Exposed mainly so tests (and curious readers) can check the packing in
 *  isolation; the bulk converter above uses the same arithmetic.
 */
uint32_t dg_color_pack32(dg_color_t c,
                         int red_shift, int green_shift, int blue_shift);

/*
 *  Pack one resolved colour into RGB565. Same rationale as dg_color_pack32.
 */
uint16_t dg_color_pack565(dg_color_t c);

#ifdef __cplusplus
}
#endif

#endif /* DOOMKIT_DG_PALETTE_H */
