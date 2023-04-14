#include "meerkat_assert/asserts.h"

#include "blender.h"

int blend_pixels_simple(PixelImage* background,
                         const MovedImage* foreground)
{
    const size_t bg_size_x = background->size.x;
    const size_t bg_size_y = background->size.y;

    const size_t fg_size_x = foreground->size.x;
    const size_t fg_size_y = foreground->size.y;

    const size_t fg_pos_x = foreground->pos.x;
    const size_t fg_pos_y = foreground->pos.y;

    SAFE_BLOCK_START
    {
        ASSERT_LESS_EQUAL(
                fg_pos_x + fg_size_x, bg_size_x);
        ASSERT_LESS_EQUAL(
                fg_pos_y + fg_size_y, bg_size_y);
    }
    SAFE_BLOCK_HANDLE_ERRORS
    {
        return -1;     
    }
    SAFE_BLOCK_END

    Pixel* bg_row = background->pixel_array + bg_size_x*fg_pos_y + fg_pos_x;
    const Pixel* fg_row = foreground->pixel_array;

    for (size_t y = 0; y < fg_size_y; ++y)
    {
        for (size_t x = 0; x < fg_size_x; ++x)
        {
            const uint16_t fg_alpha = fg_row[x].alpha;

            const uint16_t red   = bg_row[x].red   * (255 - fg_alpha)
                                 + fg_row[x].red   * fg_alpha;
            
            const uint16_t green = bg_row[x].green * (255 - fg_alpha)
                                 + fg_row[x].green * fg_alpha;

            const uint16_t blue  = bg_row[x].blue  * (255 - fg_alpha)
                                 + fg_row[x].blue  * fg_alpha;
            
            // (x >> 8) == (x / 256) ~= (x / 255)
            bg_row[x].red   = (uint8_t) (red   >> 8);
            bg_row[x].green = (uint8_t) (green >> 8);
            bg_row[x].blue  = (uint8_t) (blue  >> 8);
        }
        bg_row += bg_size_x;
        fg_row += fg_size_x;
    }

    return 0;
}
