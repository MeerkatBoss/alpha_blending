#include <immintrin.h>

#include "meerkat_assert/asserts.h"

#include "blender.h"

const char MASK_ZERO = (char) 0x80;

#define MASK_SPREAD_1_ROW \
    MASK_ZERO, 0x07,\
    MASK_ZERO, 0x06,\
    MASK_ZERO, 0x05,\
    MASK_ZERO, 0x04,\
    MASK_ZERO, 0x03,\
    MASK_ZERO, 0x02,\
    MASK_ZERO, 0x01,\
    MASK_ZERO, 0x00

#define MASK_SPREAD_2_ROW \
    MASK_ZERO, 0x0F,\
    MASK_ZERO, 0x0E,\
    MASK_ZERO, 0x0D,\
    MASK_ZERO, 0x0C,\
    MASK_ZERO, 0x0B,\
    MASK_ZERO, 0x0A,\
    MASK_ZERO, 0x09,\
    MASK_ZERO, 0x08

#define MASK_SPREAD_ALPHA_ROW \
    MASK_ZERO, 0x0E,\
    MASK_ZERO, 0x0E,\
    MASK_ZERO, 0x0E,\
    MASK_ZERO, 0x0E,\
    MASK_ZERO, 0x06,\
    MASK_ZERO, 0x06,\
    MASK_ZERO, 0x06,\
    MASK_ZERO, 0x06

#define MASK_PACK_1_ROW \
    MASK_ZERO, MASK_ZERO,\
    MASK_ZERO, MASK_ZERO,\
    MASK_ZERO, MASK_ZERO,\
    MASK_ZERO, MASK_ZERO,\
    0x0E,      0x0D,     \
    0x0B,      0x09,     \
    0x06,      0x05,     \
    0x03,      0x01

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

const __m512i EPI16_255 = _mm512_set1_epi16(0x00FF);

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
        ASSERT_TRUE(
                fg_size_x % 16 == 0);
    }
    SAFE_BLOCK_HANDLE_ERRORS
    {
        return -1;     
    }
    SAFE_BLOCK_END

    Pixel* bg_row = background->pixel_array + bg_size_x*fg_pos_y + fg_pos_x;
    Pixel* fg_row = (Pixel*) foreground->pixel_array;

    fg_row[0].alpha = 0;
    fg_row[1].alpha = 1;
    fg_row[2].alpha = 2;
    fg_row[3].alpha = 3;

    for (size_t y = 0; y < fg_size_y; ++y)
    {
        // TODO: Handle non-aligned addresses and different row sizes
        for (size_t x = 0; x < fg_size_x; x += 16)
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

            bg1 = _mm512_add_epi8(bg1, bg2);

            _mm512_storeu_si512(bg_row + x, bg1);

            int z = 0;
        }

        fg_row += fg_size_x;
        bg_row += bg_size_x;
    }

    return 0;
}
