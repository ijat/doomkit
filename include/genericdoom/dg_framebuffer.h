/* =============================================================================
 *  dg_framebuffer.h  --  Where to place DOOM's picture inside a bigger screen.
 * -----------------------------------------------------------------------------
 *  Derived from the offset math in I_FinishUpdate() in i_video.c.
 *  GNU General Public License v2. See LICENSE.
 * =============================================================================
 *
 *  THE SITUATION
 *  -------------
 *  DOOM renders a fixed 320x200 image. Your output framebuffer is usually
 *  bigger (the default is 640x400, but it could be any size). If you just
 *  copied the small image into the top-left corner it would sit in a corner
 *  with a big empty margin. Instead we usually want to:
 *
 *    - optionally enlarge the image by an integer "scaling" factor, and
 *    - centre the result, leaving an even margin on all sides.
 *
 *  This module does no copying itself. It computes the handful of byte offsets
 *  the copy loop needs -- pure arithmetic, no memory touched -- so the numbers
 *  can be reasoned about and unit-tested on their own. The actual per-row blit
 *  then just walks the framebuffer using these offsets (see
 *  examples/null/platform_null.c for a worked copy loop).
 *
 *  A PICTURE OF THE OFFSETS (one scaled row)
 *  -----------------------------------------
 *      |<-- x_offset -->|<------ row_stride ------>|<-- x_offset_end -->|
 *      +----------------+--------------------------+--------------------+
 *      |  left margin   |   the actual DOOM pixels |    right margin    |
 *      +----------------+--------------------------+--------------------+
 *
 *  All offsets are in BYTES (not pixels), because that is what a copy loop
 *  walking a `unsigned char *` needs, and it keeps the per-pixel size explicit.
 * ===========================================================================*/

#ifndef GENERICDOOM_DG_FRAMEBUFFER_H
#define GENERICDOOM_DG_FRAMEBUFFER_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  The computed layout. Every field is in bytes unless noted.
 */
typedef struct {
    int bytes_per_pixel;  /* bpp / 8 */
    int x_offset;         /* left margin before each scaled row, in bytes  */
    int x_offset_end;     /* right margin after each scaled row, in bytes   */
    int y_offset;         /* byte offset from the start of the framebuffer to
                           * the first pixel of the image -- i.e. the top
                           * margin counted as whole framebuffer rows
                           * = (top_margin_in_pixels) * fb_w * bytes_per_pixel.
                           * NOTE: the upstream engine computed this without the
                           * fb_w factor, which did not actually centre the
                           * image vertically; we compute the genuinely usable
                           * offset here. */
    int row_stride;       /* bytes of real pixels in one scaled row
                           * = screen_w * scaling * bytes_per_pixel */
    int fits;             /* 1 if the image (after scaling) fits inside the
                           * framebuffer; 0 if it would overflow (offsets are
                           * then clamped to 0 so a caller never walks before
                           * the buffer start) */
} dg_fb_layout_t;

/*
 *  Compute the centering/scaling layout.
 *    fb_w, fb_h        -- output framebuffer size in pixels (e.g. 640x400)
 *    screen_w, screen_h-- DOOM's render size in pixels (320x200)
 *    bpp               -- bits per output pixel (16 or 32 typically)
 *    scaling           -- integer enlargement factor (>= 1)
 *    out               -- filled in on return
 *
 *  The math mirrors the original engine exactly when the image fits. When it
 *  does NOT fit (framebuffer too small for the chosen scaling) the margins are
 *  clamped to zero and out->fits is set to 0, so callers can detect the problem
 *  instead of computing negative offsets.
 */
void dg_fb_layout(int fb_w, int fb_h, int screen_w, int screen_h,
                  int bpp, int scaling, dg_fb_layout_t *out);

#ifdef __cplusplus
}
#endif

#endif /* GENERICDOOM_DG_FRAMEBUFFER_H */
