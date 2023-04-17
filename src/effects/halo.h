/**
 * @file halo.h
 * @author MeerkatBoss (solodovnikov.ia@phystech.edu)
 * 
 * @brief
 *
 * @version 0.1
 * @date 2023-04-17
 *
 * @copyright Copyright MeerkatBoss (c) 2023
 */
#ifndef __HALO_H
#define __HALO_H

#include "commons/definitions.h"

/**
 * @brief Applies halo effect to the given position on image
 *
 * @param[inout] background	- Image background to apply halo to
 * @param[in]    halo	    - Halo parameters
 */
int add_halo_simple(PixelImage* background, const Halo* halo);

/**
 * @brief Applies halo effect to the given position on image
 *
 * @param[inout] background	- Image background to apply halo to
 * @param[in]    halo	    - Halo parameters
 */
int add_halo_optimized(PixelImage* background, const Halo* halo);

#endif /* halo.h */
