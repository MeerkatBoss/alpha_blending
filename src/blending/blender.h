/**
 * @file blender.h
 * @author MeerkatBoss (solodovnikov.ia@phystech.edu)
 * 
 * @brief
 *
 * @version 0.1
 * @date 2023-04-11
 *
 * @copyright Copyright MeerkatBoss (c) 2023
 */
#ifndef __BLENDER_H
#define __BLENDER_H

#include "commons/definitions.h"

/**
 * @brief Blend foreground on top of backround and store result
 * in background
 *
 * @param[inout] background	- Image background
 * @param[in]    foreground	- Image foreground
 *
 * @return 0 upon success, -1 upon invalid arguments
 */
int blend_pixels_simple(PixelImage* background,
                         const MovedImage* foreground);

/**
 * @brief Blend foreground on top of backround and store result
 * in background
 *
 * @param[inout] background	- Image background
 * @param[in]    foreground	- Image foreground
 *
 * @return 0 upon success, -1 upon invalid arguments
 */
int blend_pixels_optimized(PixelImage* background,
                            const MovedImage* foreground);

#endif /* blender.h */
