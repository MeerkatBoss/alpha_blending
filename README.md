# Alpha Blending optimization with SIMD

## Simple implementation

Initially, alpha blending algorithm was implemented without using SIMD with
regular sequential operations. Here is an extract from the main procedure of
this algorithm located in [this file](src/blending/blender_simple.cpp):

```cpp
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
```

When compiling with `-O2` optimization flags and `-mavx512f -mavx512bw` machine
flags, the compiler yields the following assembly code:

```asm
combine_pixels(Pixel*, Pixel const*):
        movzx   ecx, BYTE PTR [rsi+3]
        push    rbx
        mov     edx, ecx
        not     edx
        mov     eax, edx
        mul     BYTE PTR [rdi+1]
        mov     r8d, eax
        mov     eax, ecx
        mul     BYTE PTR [rsi+1]
        add     r8d, eax
        mov     eax, edx
        mul     BYTE PTR [rdi+2]
        mov     ebx, eax
        mov     eax, ecx
        mul     BYTE PTR [rsi+2]
        add     ebx, eax
        mov     eax, edx
        mul     BYTE PTR [rdi]
        mov     edx, eax
        mov     eax, ecx
        mul     BYTE PTR [rsi]
        mov     BYTE PTR [rdi+2], bh
        pop     rbx
        add     edx, eax
        mov     eax, r8d
        mov     BYTE PTR [rdi], dh
        mov     BYTE PTR [rdi+1], ah
        ret
```

Note, that the compiler failes to incorporate the usage of machine-specific AVX
instruction set due to the fact that the compiled procedure modifies only single
pixel and not an array of them.

## SIMD optimization

In an attempt to improve the performance of an algorithm, the main procedure
was rewritten using SIMD as follows (see
[this file](src/blending/blender_optimized.cpp) for complete code):

```cpp
__m512i combine_pixels_simd(__m512i bg1, __m512i fg1)
{
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

    return bg1;
}
```

When compiling with the same optimization level and same machine options, the
compiler yields the following assembly code:

```asm
combine_pixels_simd(long long __vector(8), long long __vector(8)):
        vpshufb zmm5, zmm1, ZMMWORD PTR MASK_SPREAD_2[rip]
        kmovd   k1, DWORD PTR IGNORE_ALPHA[rip]
        vpshufb zmm1, zmm1, ZMMWORD PTR MASK_SPREAD_1[rip]
        vmovdqa64       zmm2, ZMMWORD PTR EPI16_255[rip]
        vpshufb zmm4, zmm0, ZMMWORD PTR MASK_SPREAD_2[rip]
        vpshufb zmm6, zmm1, ZMMWORD PTR MASK_SPREAD_ALPHA[rip]
        vpshufb zmm0, zmm0, ZMMWORD PTR MASK_SPREAD_1[rip]
        vpshufb zmm3, zmm5, ZMMWORD PTR MASK_SPREAD_ALPHA[rip]
        vpsubw  zmm7, zmm2, zmm6
        vmovdqa64       zmm2, zmm0
        vpmullw zmm2{k1}, zmm7, zmm0
        vmovdqa64       zmm7, ZMMWORD PTR EPI16_255[rip]
        vpsubw  zmm0, zmm7, zmm3
        vmovdqa64       zmm7, zmm4
        vpmullw zmm7{k1}, zmm0, zmm4
        vpmullw zmm4{k1}{z}, zmm6, zmm1
        vpmullw zmm0{k1}{z}, zmm3, zmm5
        vpaddw  zmm1, zmm2, zmm4
        vpshufb zmm1, zmm1, ZMMWORD PTR MASK_PACK_1[rip]
        vpaddw  zmm0, zmm0, zmm7
        vpshufb zmm0, zmm0, ZMMWORD PTR MASK_PACK_2[rip]
        vpaddb  zmm0, zmm0, zmm1
        ret
```

The assembly code did not grow significantly in size, while gaining the ability
to process 16 pixels simultaneously. Note however, that to handle image sizes,
which are not a multiple of 16 the optimized version still needs to call a
simpler implementation, but the optimized implementation guarantees, that the
less efficient function will be called no more than 15 times per pixel row.
As the image size grows, those calls contribute less and less to the total
execution time and the performance gain increases in accordance with the
[Amdahl's Law](https://en.wikipedia.org/wiki/Amdahl%27s_law).

## Comparison results

When running performance tests on both implementation, the following results
were obtained:

| Implementation | Test time ($\pm$ stddev) |
|---|---|
| without SIMD | 469.41 ms ($\pm$ 12.94ms) |
|with SIMD | 54.51 ms ($\pm$ 1.07ms) |

Total performance increase: x8.61 ($\pm$ 0.41)


## Compiling with -O3 optimization level

When compiling the naive implementation with `-O3` optimization option, the
compiler yields [the following code](https://godbolt.org/z/MMYEcfnvr). Several
things are notable in this assembly code.

Firstly, the compiler seems to get rid of the call to `combine_pixels` entirely,
inlining this function. This clearly presents a performance gain, as no time
is spent on calling and returning from the helper function.

Secondly, the compiler successfully utilizes the vectorized instructions in the
optimized code. However, it avoids using the larger `zmm` registers, using the
smaller (by a factor of 4) `xmm` registers. While this does speed up the program
execution, we have already seen, that a greater degree of paralellism can be
achieved with `zmm` registers.

Lastly, the code compiled with `-O3` optimization level is much larger than both
previous examples. The large code size is detrimental to the program locality,
which can worsen the program performance by increasing the total number of
instruction cache misses per cycle iteration.

