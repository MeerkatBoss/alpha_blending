/**
 * @file loader.h
 * @author MeerkatBoss (solodovnikov.ia@phystech.edu)
 * 
 * @brief
 *
 * @version 0.1
 * @date 2023-04-11
 *
 * @copyright Copyright MeerkatBoss (c) 2023
 */
#ifndef __LOADER_H
#define __LOADER_H

#include "commons/definitions.h"

/**
 * @brief Load pixels of image from specified file
 *
 * @param[out] image	- Loaded image
 * @param[in]  filename	- Name of loaded image file
 *
 * @return 0 upon success, -1 otherwise
 */
int load_image_from_file(PixelImage* image, const char* filename);

/**
 * @brief Destroys loaded image. Frees associated resources.
 *
 * @param[inout] image	- Previously loaded image
 */
void unload_image(PixelImage* image);

#endif /* loader.h */

