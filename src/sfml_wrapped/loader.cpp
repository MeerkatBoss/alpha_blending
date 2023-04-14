#include <SFML/Graphics.hpp>
#include <stdlib.h>
#include <string.h>

#include "meerkat_assert/asserts.h"

#include "loader.h"

int load_image_from_file(PixelImage* image, const char* filename)
{
    SAFE_BLOCK_START    // Validate parameters
    {
        ASSERT_TRUE_MESSAGE(image    != NULL, "filename");
        ASSERT_TRUE_MESSAGE(filename != NULL, "filename");
    }
    SAFE_BLOCK_HANDLE_ERRORS
    {
        // TODO: Logs
        errno = EINVAL;
        return -1;
    }
    SAFE_BLOCK_END

    SAFE_BLOCK_START    // Load image
    {
        sf::Image sf_image;

        ASSERT_TRUE_MESSAGE(
                sf_image.loadFromFile(filename),
                "Failed to load image");
        
        sf::Vector2u size = sf_image.getSize();

        image->size.x = size.x;
        image->size.y = size.y;

        ASSERT_ZERO_MESSAGE(
                posix_memalign((void**) &image->pixel_array,
                               PIXEL_ALIGNMENT,
                               size.x * size.y * sizeof(*image->pixel_array)),
                "Failed to allocate memory");

        memcpy(image->pixel_array,
               sf_image.getPixelsPtr(),
               size.x*size.y*sizeof(*image->pixel_array));
    }
    SAFE_BLOCK_HANDLE_ERRORS
    {
        // TODO: Logs
        return -1;
    }
    SAFE_BLOCK_END

    return 0;
}

void unload_image(PixelImage* image)
{
    SAFE_BLOCK_START
    {
        ASSERT_TRUE_MESSAGE(image != NULL, "image");
        ASSERT_TRUE_MESSAGE(image->pixel_array != NULL, "image->pixel_array");
    }
    SAFE_BLOCK_HANDLE_ERRORS
    {
        // TODO: Logs
        return;
    }
    SAFE_BLOCK_END

    free(image->pixel_array);
    
    image->pixel_array = NULL;
    image->size.x = 0;
    image->size.y = 0;
}

