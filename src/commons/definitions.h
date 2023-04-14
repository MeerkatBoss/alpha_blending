/**
 * @file definitions.h
 * @author MeerkatBoss (solodovnikov.ia@phystech.edu)
 * 
 * @brief
 *
 * @version 0.1
 * @date 2023-04-10
 *
 * @copyright Copyright MeerkatBoss (c) 2023
 */
#ifndef __ALPHA_DEFINITIONS_H
#define __ALPHA_DEFINITIONS_H

#include <stdint.h>
#include <stddef.h>

#define PIXEL_ALIGNMENT 64
#define FONTNAME "LobsterTwo-BoldItalic"

struct Pixel
{
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    uint8_t alpha;
};

struct SizeVector2
{
    size_t x;
    size_t y;
};

struct PixelImage
{
    SizeVector2 size;
    
    Pixel* pixel_array;
};

struct MovedImage
{
    SizeVector2 size;
    SizeVector2 pos;

    Pixel* pixel_array;
};

struct RenderConfig
{
    SizeVector2 fg_pos;

    const char* fg_image_name;
    const char* bg_image_name;
    const char* font_name;
};

#endif /* definitions.h */
