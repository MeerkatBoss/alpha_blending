#include <stdio.h>

#include "meerkat_assert/asserts.h"
#include "sfml_wrapped/display.h"

int main()
{
    const RenderConfig config = {
        .fg_pos = { 544, 278 },
        .fg_image_name = "assets/poltorashka_cropped_medium.bmp",
        .bg_image_name = "assets/wooden_table_scaled.bmp",
        .font_name     = "assets/" FONTNAME ".ttf"
    };
    RenderScene scene = {};

    SAFE_BLOCK_START
    {
        ASSERT_ZERO(
                render_scene_init(&scene, &config));
    }
    SAFE_BLOCK_HANDLE_ERRORS
    {
        return 1;
    }
    SAFE_BLOCK_END

    run_main_loop(&scene);
    render_scene_dispose(&scene);

    return 0;
}
