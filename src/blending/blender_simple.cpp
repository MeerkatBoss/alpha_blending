#include "meerkat_assert/asserts.h"

#include "blender.h"

void combine_pixels(Pixel* bg, const Pixel* fg)
{
    const uint16_t fg_alpha = fg->alpha;

    const uint16_t red   = bg->red   * (255 - fg_alpha)
                         + fg->red   * fg_alpha;
    
    const uint16_t green = bg->green * (255 - fg_alpha)
                         + fg->green * fg_alpha;

    const uint16_t blue  = bg->blue  * (255 - fg_alpha)
                         + fg->blue  * fg_alpha;
    
    // (x >> 8) == (x / 256) ~= (x / 255)
    bg->red   = (uint8_t) (red   >> 8);
    bg->green = (uint8_t) (green >> 8);
    bg->blue  = (uint8_t) (blue  >> 8);
}

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
            combine_pixels(bg_row + x, fg_row + x);
        }
        bg_row += bg_size_x;
        fg_row += fg_size_x;
    }

    return 0;
}
