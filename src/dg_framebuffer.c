/* =============================================================================
 *  dg_framebuffer.c  --  Centering / scaling offset math.
 *  See include/genericdoom/dg_framebuffer.h for the diagram and rationale.
 *  Derived from the offset calculation in I_FinishUpdate() in i_video.c.
 *  GNU General Public License v2. See LICENSE.
 * ===========================================================================*/

#include "genericdoom/dg_framebuffer.h"

void dg_fb_layout(int fb_w, int fb_h, int screen_w, int screen_h,
                  int bpp, int scaling, dg_fb_layout_t *out)
{
    int bytes_per_pixel = bpp / 8;

    /* Size of the image once enlarged, in pixels. */
    int scaled_w = screen_w * scaling;
    int scaled_h = screen_h * scaling;

    /* Leftover space around the image, in pixels (can be negative if the
     * framebuffer is too small for the requested scaling). */
    int spare_w_px = fb_w - scaled_w;
    int spare_h_px = fb_h - scaled_h;

    out->bytes_per_pixel = bytes_per_pixel;
    out->row_stride = scaled_w * bytes_per_pixel;

    if (spare_w_px < 0 || spare_h_px < 0) {
        /* The image does not fit. Rather than hand back negative offsets (which
         * a copy loop would turn into an out-of-bounds write), clamp the
         * margins to zero and flag the caller. */
        out->x_offset = 0;
        out->x_offset_end = 0;
        out->y_offset = 0;
        out->fits = 0;
        return;
    }

    /* It fits: split the leftover space evenly to centre the image. The width
     * margins are computed in bytes (matching the original engine), and
     * x_offset_end is whatever is left after the left margin so that any odd
     * remainder lands on the right -- exactly as I_FinishUpdate did it. */
    int spare_w_bytes = spare_w_px * bytes_per_pixel;
    out->x_offset = spare_w_bytes / 2;
    out->x_offset_end = spare_w_bytes - out->x_offset;

    /* Vertical centring: half the spare rows go on top. We express that as a
     * byte offset to the first image pixel, which is top_margin_rows worth of
     * FULL framebuffer rows -- hence the * fb_w. (The upstream engine omitted
     * the fb_w factor; we fix it so the image is actually centred.) */
    int top_margin_rows = spare_h_px / 2;
    out->y_offset = top_margin_rows * fb_w * bytes_per_pixel;
    out->fits = 1;
}
