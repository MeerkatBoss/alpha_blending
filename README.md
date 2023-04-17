# Alpha Blending optimization with SIMD

## Simple implementation

Initially, alpha blending algorithm was implemented without using SIMD with
regular sequential operations. Here is an extract from the main procedure of
this algorithm located in [this file](src/blending/blender_simple.cpp):

```cpp
const uint16_t fg_alpha = fg_row[x].alpha;

const uint16_t red   = bg_row[x].red   * (255 - fg_alpha)
                     + fg_row[x].red   * fg_alpha;

const uint16_t green = bg_row[x].green * (255 - fg_alpha)
                     + fg_row[x].green * fg_alpha;

const uint16_t blue  = bg_row[x].blue  * (255 - fg_alpha)
                     + fg_row[x].blue  * fg_alpha;

bg_row[x].red   = (uint8_t) (red   >> 8);
bg_row[x].green = (uint8_t) (green >> 8);
bg_row[x].blue  = (uint8_t) (blue  >> 8);
```

When compiling with `-O2` optimization flags and `-mavx512f -mavx512bw` machine
flags, the compiler yields the following assembly code:

```asm
movzx   eax, BYTE PTR [rbx+3]
movzx   r15d, BYTE PTR [rdx+1]
mov     esi, r10d
add     rdx, 4
movzx   r14d, BYTE PTR [rdx-2]
add     rbx, 4
sub     esi, eax
mov     ecx, eax
imul    r15d, esi
mul     BYTE PTR [rbx-3]
imul    r14d, esi
add     r15d, eax
mov     eax, ecx
mul     BYTE PTR [rbx-2]
add     r14d, eax
movzx   eax, BYTE PTR [rdx-4]
imul    esi, eax
mov     eax, ecx
mul     BYTE PTR [rbx-4]
add     eax, esi
mov     BYTE PTR [rdx-4], ah
mov     eax, r15d
mov     BYTE PTR [rdx-3], ah
mov     eax, r14d
mov     BYTE PTR [rdx-2], ah
```

Note, that on `-O2` optimization level the compiler failes to incorporate the
usage of machine-specific AVX instruction set.

## SIMD optimization

In an attempt to improve the performance of an algorithm, the main procedure
was rewritten using SIMD as follows (see
[this file](src/blending/blender_optimized.cpp) for complete code):

```cpp
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

...

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
```

When compiling with the same optimization level and same machine options, the
compiler yields the following assembly code:

```asm
vmovdqu8        zmm4, ZMMWORD PTR [r9-64+rax*4]
vmovdqu8        zmm3, ZMMWORD PTR [rdx-64+rax*4]
mov     rcx, rax
vmovdqu8        zmm2, ZMMWORD PTR [r9-64+rax*4]
vpshufb zmm0, zmm4, zmm8
vpshufb zmm1, zmm3, zmm8
vpshufb zmm5, zmm3, zmm9
vpshufb zmm4, zmm0, zmm7
vpshufb zmm2, zmm2, zmm9
vmovdqa64       zmm11, zmm1
vpshufb zmm3, zmm2, zmm7
vpsubw  zmm10, zmm6, zmm4
vpmullw zmm11{k1}, zmm10, zmm1
vpsubw  zmm1, zmm6, zmm3
vmovdqa64       zmm10, zmm5
vpmullw zmm10{k1}, zmm1, zmm5
vpmullw zmm1{k1}{z}, zmm4, zmm0
vpmullw zmm0{k1}{z}, zmm3, zmm2
vpaddw  zmm1, zmm1, zmm11
vpaddw  zmm0, zmm0, zmm10
vpshufb zmm1, zmm1, zmm13
vpshufb zmm0, zmm0, zmm12
vpaddb  zmm0, zmm0, zmm1
vmovdqu64       ZMMWORD PTR [rdx-64+rax*4], zmm0

...

movzx   eax, BYTE PTR [rsi+3]
movzx   r15d, BYTE PTR [rdx+1]
mov     r8d, r11d
add     rdx, 4
movzx   ebx, BYTE PTR [rdx-2]
add     rsi, 4
sub     r8d, eax
mov     ecx, eax
imul    r15d, r8d
mul     BYTE PTR [rsi-3]
imul    ebx, r8d
add     r15d, eax
mov     eax, ecx
mul     BYTE PTR [rsi-2]
add     ebx, eax
movzx   eax, BYTE PTR [rdx-4]
imul    r8d, eax
mov     eax, ecx
mul     BYTE PTR [rsi-4]
mov     BYTE PTR [rdx-2], bh
add     eax, r8d
mov     BYTE PTR [rdx-4], ah
mov     eax, r15d
mov     BYTE PTR [rdx-3], ah
```

The size of assembly code of fragment of code performing actual computations
grew by about a factor of two, due to the necessity of handling image sizes,
which are not a multiple of 16. However this does not present a big enough
problem, as code size remains relatively small and therefore cache-friendly.

The second implementation is able to perform 16 pixel computations per cycle
iteration, which makes it significantly faster than the first one. Note, that
the second, less efficient cycle in the efficient implementation is the exact
copy of the cycle in naive implementation, however it is guaranteed to execute
no more than 15 times per image row ($O(1)$ time). As the image size grows,
smaller and smaller percentage of pixels will be calculated inefficiently and
the performance gain will grow in accordance with the
[Amdahl's Law](https://en.wikipedia.org/wiki/Amdahl%27s_law).

## Compiling with -O3 optimization level

When compiling the naive implementation with `-O3` optimization option, the
compiler yields [the following assembly code](https://godbolt.org/z/a3aMbK66d).
Two things are notable here. Firstly, the size of assembly code grew
significantly in comparison with the `-O2` compiled version. Secondly, the
compiler was able to utilize the vectorized instructions, however it seems to
have used not the larger `zmm` registers, but the smaller (by a factor of 4)
`xmm` registers, which would not be able to outperform the optimized version.
Additionally, the increased code size undermines to locality of the program,
which in turn decreases the probability of instruction cache hits and slows
the program execution down.

When compiling the optimized implementation with the `-O3` optimization level,
the produced assembly code does not change compared to the `-O2` compiled
version.

