#include <math.h>

#include "meerkat_assert/asserts.h"

#include "blending/blender.h"

#include "halo.h"

int add_halo_simple(PixelImage* background, const Halo* halo)
{
    SAFE_BLOCK_START
    {
        ASSERT_TRUE(background != NULL);

        ASSERT_TRUE(halo != NULL);

        ASSERT_LESS(
                halo->center.x + halo->radius_px,
                background->size.x);

        ASSERT_LESS(
                halo->center.y + halo->radius_px,
                background->size.y);

        ASSERT_GREATER_EQUAL(
                halo->center.x, halo->radius_px);

        ASSERT_GREATER_EQUAL(
                halo->center.y, halo->radius_px);
    }
    SAFE_BLOCK_HANDLE_ERRORS
    {
        // TODO: Logs
        return -1;
    }
    SAFE_BLOCK_END

    Pixel blended = {
        .red   = halo->color.red,
        .green = halo->color.green,
        .blue  = halo->color.blue,
        .alpha = 0
    };

    double alpha_norm = (double)halo->color.alpha / 255;

    const size_t radius = halo->radius_px;
    const double radius_sq = (double) radius * (double) radius;

    const size_t bg_size_x = background->size.x;

    const size_t center_x  = halo->center.x;
    const size_t center_y  = halo->center.y;

    Pixel* bg_row = background->pixel_array
                    + (center_y-radius) * bg_size_x
                    + center_x - radius;

    for (size_t y = 0; y <= radius*2; y++)
    {
        for (size_t x = 0; x <= radius*2; x++) // TODO: Extract
        {
            double dx = (double) (x > radius
                                  ? x - radius
                                  : radius - x);
            double dy = (double) (y > radius
                                  ? y - radius
                                  : radius - y);
            
            double color_base = radius_sq - (dx*dx + dy*dy);

            if (color_base < 0) continue;

            // color_base /= radius_sq;
            color_base *= alpha_norm;

            // color_base is intentionally NOT normalized, int overflow
            // produces really cool stripes
            // uint8_t alpha = (uint8_t) (unsigned) (255 * sqrt(color_base));

            uint8_t alpha = (uint8_t) (255 * color_base);
            blended.alpha = alpha;

            combine_pixels(bg_row + x, &blended);
        }
        bg_row += bg_size_x;
    }

    return 0;
}

