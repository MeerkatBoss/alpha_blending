#include <stdlib.h>
#include <string.h>

#include "meerkat_assert/asserts.h"
#include "sfml_wrapped/loader.h"
#include "blending/blender.h"

#include "display.h"

static int load_fonts     (RenderScene* scene, const RenderConfig* config);
static int load_images    (RenderScene* scene, const RenderConfig* config);
static int allocate_pixels(RenderScene* scene);

int render_scene_init(RenderScene* scene, const RenderConfig* config)
{
    SAFE_BLOCK_START        // Only these operations can fail
    {
        ASSERT_ZERO(
                load_fonts(scene, config));
        ASSERT_ZERO(
                load_images(scene, config));
        ASSERT_ZERO(
                allocate_pixels(scene));
    }
    SAFE_BLOCK_HANDLE_ERRORS
        return -1;
    SAFE_BLOCK_END

    scene->pos.x = config->fg_pos.x;
    scene->pos.y = config->fg_pos.y;

    const unsigned window_width  = (unsigned) scene->background.size.x;
    const unsigned window_height = (unsigned) scene->background.size.y;

    scene->display_texture.create(window_width, window_height);
    scene->display_sprite.setTexture(scene->display_texture);

    new (&scene->window) sf::RenderWindow(
            sf::VideoMode(window_width, window_height),
            "Alpha Blending",
            sf::Style::Titlebar | sf::Style::Close);

    return 0;
}

void render_scene_dispose(RenderScene* scene)
{
    free(scene->texture_pixels);
    scene->texture_pixels = 0;

    unload_image(&scene->foreground);
    unload_image(&scene->background);
}

void run_main_loop(RenderScene* scene)
{
    char buffer[16] = "";

    size_t bg_size_x = scene->background.size.x;
    size_t bg_size_y = scene->background.size.y;

    const MovedImage moved_fg = {
        .size = {
            .x = scene->foreground.size.x,
            .y = scene->foreground.size.y,
        },
        .pos = {
            .x = scene->pos.x,
            .y = scene->pos.y
        },
        .pixel_array = scene->foreground.pixel_array
    };
    PixelImage texture_image = {
        .size = {
            .x = scene->background.size.x,
            .y = scene->background.size.y,
        },
        .pixel_array = scene->texture_pixels
    };

    sf::Clock clock;
    while (scene->window.isOpen())
    {
        sf::Event event;
        while (scene->window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                scene->window.close();
        }

        scene->window.clear(sf::Color::White);
        float timeDelta = clock.restart().asSeconds();
        snprintf(buffer, 16, "%.1f FPS", 1.f/timeDelta);
        scene->fps_text.setString(buffer);

        memcpy(scene->texture_pixels, scene->background.pixel_array,
                bg_size_x*bg_size_y*sizeof(*scene->texture_pixels));

        blend_pixels_optimized(&texture_image, &moved_fg);
        scene->display_texture.update(
                (const sf::Uint8*) scene->texture_pixels);

        scene->window.draw(scene->display_sprite);
        scene->window.draw(scene->fps_text);

        scene->window.display();
    }
}

static int load_fonts(RenderScene* scene, const RenderConfig* config)
{
    SAFE_BLOCK_START    // Validate parameters
    {
        ASSERT_TRUE_MESSAGE(config->font_name != NULL, "font_name");
    }
    SAFE_BLOCK_HANDLE_ERRORS
    {
        // TODO: Logs
        errno = EINVAL;
        return -1;
    }
    SAFE_BLOCK_END

    SAFE_BLOCK_START    // Load font file
    {
        ASSERT_TRUE_MESSAGE(
            scene->fps_font.loadFromFile(config->font_name),
            /* message */ config->font_name); 

        scene->fps_text.setFont(scene->fps_font);
        scene->fps_text.setFillColor(sf::Color::White);
    }
    SAFE_BLOCK_HANDLE_ERRORS
    {
        // TODO: Add logs
        return -1;
    }
    SAFE_BLOCK_END

    return 0;
}

static int load_images(RenderScene* scene, const RenderConfig* config)
{
    // TODO: Maybe the whole "Validate parameters" idiom can be extracted.
    //          However, that would require both assertions and logs.

    SAFE_BLOCK_START    // Validate parameters
    {
        ASSERT_TRUE_MESSAGE(config->fg_image_name != NULL, "fg_image_name");
        ASSERT_TRUE_MESSAGE(config->bg_image_name != NULL, "bg_image_name");
    }
    SAFE_BLOCK_HANDLE_ERRORS
    {
        // TODO: Add logs
        errno = EINVAL;
        return -1;
    }
    SAFE_BLOCK_END

    SAFE_BLOCK_START    // Load files
    {
        // Foreground
        ASSERT_MESSAGE(
                load_image_from_file(&scene->foreground,
                                     config->fg_image_name),
                action_result == 0,
                /* message */ config->fg_image_name);

        // Background
        ASSERT_MESSAGE(
                load_image_from_file(&scene->background,
                                     config->bg_image_name),
                action_result == 0,
                /* message */ config->bg_image_name);
    }
    SAFE_BLOCK_HANDLE_ERRORS
    {
        // TODO: Add logs
        return -1;
    }
    SAFE_BLOCK_END

    return 0;
}

static int allocate_pixels(RenderScene* scene)
{
    const size_t window_width  = scene->background.size.x;
    const size_t window_height = scene->background.size.y;

    SAFE_BLOCK_START    // Validate input
    {
        ASSERT_POSITIVE_MESSAGE(window_width,  "window_width");
        ASSERT_POSITIVE_MESSAGE(window_height, "window_height");
    }
    SAFE_BLOCK_HANDLE_ERRORS
    {
        // TODO: Add logs
        errno = EINVAL;
        return -1;
    }
    SAFE_BLOCK_END
        
    SAFE_BLOCK_START    // Perform allocation
    {
        const size_t array_size = window_width * window_height;

        ASSERT_TRUE_MESSAGE_CALLBACK(
                array_size / window_width == window_height,
                "Integer multiplication overflow",
                errno = EOVERFLOW);

        ASSERT_ZERO_MESSAGE(
            posix_memalign(
                    (void**) &scene->texture_pixels,
                    PIXEL_ALIGNMENT,
                    window_height*window_width*sizeof(*scene->texture_pixels)),
            "Failed to allocate memory");
    }
    SAFE_BLOCK_HANDLE_ERRORS
    {
        // TODO: Add logs
        return -1;
    }
    SAFE_BLOCK_END
    
    return 0;
}
