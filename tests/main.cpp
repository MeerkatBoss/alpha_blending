#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "commons/definitions.h"
#include "sfml_wrapped/loader.h"
#include "blending/blender.h"

#include "helpers/test_macros.h"

struct test_args
{
    PixelImage* background;
    MovedImage* foreground;
};

#define EXPAND_ARGS(args) (args.background, args.foreground)
#define ADAPTER(function) function EXPAND_ARGS

int main()
{
    PixelImage foreground = {}, background = {};

    load_image_from_file(&foreground, "assets/poltorashka_cropped.bmp");
    load_image_from_file(&background, "assets/wooden_table_scaled.bmp");
    
    MovedImage moved_fg = {
        .size = {foreground.size.x, foreground.size.y},
        .pos = {0, 0},
        .pixel_array = foreground.pixel_array
    };

    const size_t sample_size = 500;
    const size_t repeat = 300;
    size_t* test_data = (size_t*) calloc(sample_size, sizeof(*test_data));
    
    const test_args args = { &background, &moved_fg };

    COLLECT_DATA(ADAPTER(blend_pixels_simple), args, repeat,
                    test_data, sample_size);

    const double simple_average = get_average_time(test_data, sample_size);
    const double simple_stddev  = get_time_standard_deviation(simple_average,
                                                        test_data, sample_size);

    COLLECT_DATA(ADAPTER(blend_pixels_optimized), args, repeat,
                    test_data, sample_size);

    const double opt_average = get_average_time(test_data, sample_size);
    const double opt_stddev  = get_time_standard_deviation(opt_average,
                                                        test_data, sample_size);

    const double simple_rel_error = simple_stddev / simple_average;
    const double opt_rel_error    = opt_stddev    / opt_average;
    const double combined_rel_error = simple_rel_error + opt_rel_error;
    const double faster = simple_average / opt_average;
    const double faster_err = faster * combined_rel_error;


    printf("without SIMD: %.2lfms (~%.2lfms)\n", simple_average, simple_stddev);
    printf("   with SIMD: %.2lfms (~%.2lfms)\n", opt_average, opt_stddev);
    puts("");
    printf("Performance increase: %.2lf (~%.2lf)\n", faster, faster_err);

    unload_image(&foreground);
    unload_image(&background);

    free(test_data);
}
