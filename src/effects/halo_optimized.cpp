#include <immintrin.h>
#include <math.h>

#include "meerkat_assert/asserts.h"

#include "blending/blender.h"

#include "halo.h"

#define MASK_ZERO ((char)0x80)
#define SHUFFLE_MASK_ROW \
    0x0C, MASK_ZERO, MASK_ZERO, MASK_ZERO,\
    0x08, MASK_ZERO, MASK_ZERO, MASK_ZERO,\
    0x04, MASK_ZERO, MASK_ZERO, MASK_ZERO,\
    0x00, MASK_ZERO, MASK_ZERO, MASK_ZERO

const __m512i SHUFFLE_MASK = _mm512_set_epi8(
                                SHUFFLE_MASK_ROW,
                                SHUFFLE_MASK_ROW,
                                SHUFFLE_MASK_ROW,
                                SHUFFLE_MASK_ROW);

const __mmask64 BLEND_MASK = _cvtu64_mask64(0x7777777777777777);

int add_halo_optimized(PixelImage* background, const Halo* halo)
{
    SAFE_BLOCK_START
    {
        ASSERT_TRUE(background != NULL);

        ASSERT_TRUE(halo != NULL);

        // ASSERT_TRUE(halo->radius_px % 16 == 0);

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
    
    const __m512 alpha_norm = _mm512_div_ps(
                        _mm512_set1_ps((float)halo->color.alpha),
                        _mm512_set1_ps(255));

    const size_t side_length = 2 * halo->radius_px;
    const __m512 radius = _mm512_set1_ps((float)halo->radius_px);
    const __m512 radius_sq = _mm512_mul_ps(radius, radius);

    const size_t bg_size_x = background->size.x;

    const size_t center_x  = halo->center.x;
    const size_t center_y  = halo->center.y;

    Pixel* bg_row = background->pixel_array
                    + (center_y - side_length/2) * bg_size_x
                    + center_x - side_length/2;

    __m512i blended = _mm512_set1_epi32(*(const int*)&halo->color);
    __m512 y_coord = _mm512_set1_ps(0);

    for (size_t y = 0; y <= side_length; y++)
    {
        size_t x = 0;

        __m512 x_coord = _mm512_setr_ps(
                0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);

        for (; x + 16 <= side_length; x += 16) // TODO: Extract
        {
            __m512 dx = _mm512_sub_ps(x_coord, radius);
            __m512 dy = _mm512_sub_ps(y_coord, radius);
            __m512 dist_sq = _mm512_add_ps(
                                _mm512_mul_ps(dx, dx),
                                _mm512_mul_ps(dy, dy));
            __m512 color_base = _mm512_sub_ps(radius_sq, dist_sq);

            __mmask16 cmp_mask = _mm512_cmp_ps_mask(
                                    color_base, _mm512_set1_ps(0), _CMP_GE_OS);

            color_base = _mm512_maskz_mul_ps(cmp_mask, color_base, alpha_norm);

            // color_base is intentionally NOT normalized, int overflow
            // produces really cool stripes
            // color_base = _mm512_sqrt_ps(color_base);
            color_base = _mm512_div_ps(color_base, radius_sq);
            color_base = _mm512_mul_ps(color_base, _mm512_set1_ps(255));
            __m512i alpha = _mm512_cvt_roundps_epi32(color_base,
                                _MM_FROUND_TO_NEAREST_INT |_MM_FROUND_NO_EXC);

            // Lower bytes to higher
            alpha = _mm512_shuffle_epi8(alpha, SHUFFLE_MASK);

            // Set new alpha channel, leave others as is
            blended = _mm512_mask_blend_epi8(BLEND_MASK, alpha, blended);

            __m512i bg = _mm512_loadu_si512(bg_row + x);
            __m512i result = combine_pixels_simd(bg, blended);
            _mm512_storeu_si512(bg_row + x, result);

            x_coord = _mm512_add_ps(x_coord, _mm512_set1_ps(16));
        }

        for (; x < side_length; x++) // TODO: Extract
        {
            size_t rad = side_length / 2;
            double dx = (double) (x > rad
                                  ? x - rad
                                  : rad - x);
            double dy = (double) (y > rad
                                  ? y - rad
                                  : rad - y);
            
            double color_base = rad*rad - (dx*dx + dy*dy);

            if (color_base < 0) continue;

            color_base /= rad*rad;
            // color_base *= (double) halo->color.alpha / 255; 

            // color_base is intentionally NOT normalized, int overflow
            // produces really cool stripes
            // uint8_t alpha = (uint8_t) (unsigned) (255 * sqrt(color_base));

            uint8_t alpha = (uint8_t) (255 * color_base);
            const Pixel to_blend = {
                halo->color.red,
                halo->color.green,
                halo->color.blue,
                alpha
            };

            combine_pixels(bg_row + x, &to_blend);

        }
        bg_row += bg_size_x;

        y_coord = _mm512_add_ps(y_coord, _mm512_set1_ps(1));
    }

    return 0;
}

