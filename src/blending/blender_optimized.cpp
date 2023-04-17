#include <immintrin.h>

#include "meerkat_assert/asserts.h"

#include "blender.h"

const char MASK_ZERO = (char) 0x80;

/*
 * [ r0 g0 b0 a0 | r1 g1 b1 a1 | r2 g2 b2 a2 | r3 g3 b3 a3 ]
 *                             V
 *                             V
 * [ r0 00 g0 00   b0 00 a0 00 | r1 00 g1 00   b1 00 a1 00 ]
 */
#define MASK_SPREAD_1_ROW \
    MASK_ZERO, 0x07,\
    MASK_ZERO, 0x06,\
    MASK_ZERO, 0x05,\
    MASK_ZERO, 0x04,\
    MASK_ZERO, 0x03,\
    MASK_ZERO, 0x02,\
    MASK_ZERO, 0x01,\
    MASK_ZERO, 0x00

/*
 * [ r0 g0 b0 a0 | r1 g1 b1 a1 | r2 g2 b2 a2 | r3 g3 b3 a3 ]
 *                             V
 *                             V
 * [ r2 00 g2 00   b2 00 a2 00 | r3 00 g3 00   b3 00 a3 00 ]
 */
#define MASK_SPREAD_2_ROW \
    MASK_ZERO, 0x0F,\
    MASK_ZERO, 0x0E,\
    MASK_ZERO, 0x0D,\
    MASK_ZERO, 0x0C,\
    MASK_ZERO, 0x0B,\
    MASK_ZERO, 0x0A,\
    MASK_ZERO, 0x09,\
    MASK_ZERO, 0x08

/*
 * [ r0 00 g0 00 b0 00 a0 00 | r1 00 g1 00 b1 00 a1 00 ]
 *                           V
 *                           V
 * [ a0 00 a0 00 a0 00 a0 00 | a1 00 a1 00 a1 00 a1 00 ]
 */
#define MASK_SPREAD_ALPHA_ROW \
    MASK_ZERO, 0x0E,\
    MASK_ZERO, 0x0E,\
    MASK_ZERO, 0x0E,\
    MASK_ZERO, 0x0E,\
    MASK_ZERO, 0x06,\
    MASK_ZERO, 0x06,\
    MASK_ZERO, 0x06,\
    MASK_ZERO, 0x06

/*
 * Because alpha channel is not updated, it resides in lower byte
 * of half-word, unlike other channels
 *
 * [ xx r0 xx g0   xx b0 a0 00 | xx r1 xx g1   xx b1 a1 00 ]
 *                             V
 *                             V
 * [ r0 g0 b0 a0 | r1 g1 b1 a1 | 00 00 00 00 | 00 00 00 00 ]
 */
#define MASK_PACK_1_ROW \
    MASK_ZERO, MASK_ZERO,\
    MASK_ZERO, MASK_ZERO,\
    MASK_ZERO, MASK_ZERO,\
    MASK_ZERO, MASK_ZERO,\
    0x0E,      0x0D,     \
    0x0B,      0x09,     \
    0x06,      0x05,     \
    0x03,      0x01

/*
 * Because alpha channel is not updated, it resides in lower byte
 * of half-word, unlike other channels
 *
 * [ xx r2 xx g2   xx b2 a2 00 | xx r2 xx g2   xx b2 a2 00 ]
 *                             V
 *                             V
 * [ 00 00 00 00 | 00 00 00 00 | r2 g2 b2 a2 | r3 g3 b3 a3 ]
 */
#define MASK_PACK_2_ROW \
    0x0E,      0x0D,     \
    0x0B,      0x09,     \
    0x06,      0x05,     \
    0x03,      0x01,     \
    MASK_ZERO, MASK_ZERO,\
    MASK_ZERO, MASK_ZERO,\
    MASK_ZERO, MASK_ZERO,\
    MASK_ZERO, MASK_ZERO

const __m512i MASK_SPREAD_1 = _mm512_set_epi8(
    MASK_SPREAD_1_ROW,
    MASK_SPREAD_1_ROW,
    MASK_SPREAD_1_ROW,
    MASK_SPREAD_1_ROW
);

const __m512i MASK_SPREAD_2 = _mm512_set_epi8(
    MASK_SPREAD_2_ROW,
    MASK_SPREAD_2_ROW,
    MASK_SPREAD_2_ROW,
    MASK_SPREAD_2_ROW
);

const __m512i MASK_SPREAD_ALPHA = _mm512_set_epi8(
    MASK_SPREAD_ALPHA_ROW,
    MASK_SPREAD_ALPHA_ROW,
    MASK_SPREAD_ALPHA_ROW,
    MASK_SPREAD_ALPHA_ROW
);

const __m512i MASK_PACK_1 = _mm512_set_epi8(
    MASK_PACK_1_ROW,
    MASK_PACK_1_ROW,
    MASK_PACK_1_ROW,
    MASK_PACK_1_ROW
);

const __m512i MASK_PACK_2 = _mm512_set_epi8(
    MASK_PACK_2_ROW,
    MASK_PACK_2_ROW,
    MASK_PACK_2_ROW,
    MASK_PACK_2_ROW
);

// All half-words set to 255
const __m512i EPI16_255 = _mm512_set1_epi16(0x00FF);

// During calculations, alpha channel of background should not be affected
const __mmask32 IGNORE_ALPHA = _cvtu32_mask32(0x77777777);

#undef MASK_SPREAD_1_ROW
#undef MASK_SPREAD_2_ROW
#undef MASK_SPREAD_ALPHA_ROW
#undef MASK_PACK_1_ROW
#undef MASK_PACK_2_ROW

int blend_pixels_optimized(PixelImage* background,
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
        // TODO: Handle non-aligned addresses and different row sizes
        size_t x = 0;
        for (x = 0; x + 16 <= fg_size_x; x += 16)
        {
            __m512i fg1 = _mm512_loadu_si512(fg_row + x);
            __m512i bg1 = _mm512_loadu_si512(bg_row + x);

            __m512i fg2 = _mm512_shuffle_epi8(fg1, MASK_SPREAD_2);
            __m512i bg2 = _mm512_shuffle_epi8(bg1, MASK_SPREAD_2);
                    fg1 = _mm512_shuffle_epi8(fg1, MASK_SPREAD_1);
                    bg1 = _mm512_shuffle_epi8(bg1, MASK_SPREAD_1);

            __m512i fg_alpha1 = _mm512_shuffle_epi8(fg1, MASK_SPREAD_ALPHA);
            __m512i fg_alpha2 = _mm512_shuffle_epi8(fg2, MASK_SPREAD_ALPHA);

            __m512i bg_alpha1 = _mm512_sub_epi16(EPI16_255, fg_alpha1);
            __m512i bg_alpha2 = _mm512_sub_epi16(EPI16_255, fg_alpha2);

            bg1 = _mm512_mask_mullo_epi16(bg1, IGNORE_ALPHA, bg1, bg_alpha1);
            bg2 = _mm512_mask_mullo_epi16(bg2, IGNORE_ALPHA, bg2, bg_alpha2);

            fg1 = _mm512_maskz_mullo_epi16(IGNORE_ALPHA, fg1, fg_alpha1);
            fg2 = _mm512_maskz_mullo_epi16(IGNORE_ALPHA, fg2, fg_alpha2);

            bg1 = _mm512_add_epi16(bg1, fg1);
            bg2 = _mm512_add_epi16(bg2, fg2);

            bg1 = _mm512_shuffle_epi8(bg1, MASK_PACK_1);
            bg2 = _mm512_shuffle_epi8(bg2, MASK_PACK_2);

            // Pixels do not intersect and can be simply added
            bg1 = _mm512_add_epi8(bg1, bg2);

            _mm512_storeu_si512(bg_row + x, bg1);
        }
        // Remaining pixels
        for (; x < fg_size_x; ++x)
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

        fg_row += fg_size_x;
        bg_row += bg_size_x;
    }

    return 0;
}
