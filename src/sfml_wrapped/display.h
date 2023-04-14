/**
 * @file display.h
 * @author MeerkatBoss (solodovnikov.ia@phystech.edu)
 * 
 * @brief
 *
 * @version 0.1
 * @date 2023-04-11
 *
 * @copyright Copyright MeerkatBoss (c) 2023
 */
#ifndef __DISPLAY_H
#define __DISPLAY_H

#include <SFML/Graphics.hpp>

#include "commons/definitions.h"

struct RenderScene
{
    sf::RenderWindow    window;   
    
    SizeVector2         pos;
    PixelImage          foreground;
    PixelImage          background;

    Pixel*              texture_pixels;
    sf::Texture         display_texture;
    sf::Sprite          display_sprite;

    sf::Font            fps_font;
    sf::Text            fps_text;
};

/**
 * @brief Initialize scene state for future rendering
 *
 * @param[out] scene	        - scene to be initialized
 * @param[in]  config	        - rendering configuration
 *
 * @return 0 upon success, -1 upon error
 */
int render_scene_init(RenderScene* scene, const RenderConfig* config);

/**
 * @brief Free all memory, allocated in `render_scene_init`.
 * Scene can be safely destroyed afterwards.
 *
 * @param[inout] scene	        - scene to be disposed
 */
void render_scene_dispose(RenderScene* scene);

/**
 * @brief Run main loop on given render scene
 *
 * @param[in] scene	            - active render scene
 */
void run_main_loop(RenderScene* scene);

#endif /* display.h */
