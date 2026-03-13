#ifndef SRC_GRAPHICS_COMMON_H
#define SRC_GRAPHICS_COMMON_H

#define INTERPOLATION_TYPES_COUNT 3

#define INTERPOLATION_NEAR 0
#define INTERPOLATION_LINEAR 1
#define INTERPOLATION_LANCZOS 2

#define LANCZOS_A 1

typedef struct coordinates coordinates;
struct coordinates {
    int x;
    int y;
};

typedef struct {
    unsigned char red;
    unsigned char green;
    unsigned char blue;
} color_rgb;

typedef struct
{
    coordinates top_left;
    coordinates bottom_right;
} rectangle;

typedef struct {
    char *name;
    char *description;
} interpolation_description;

extern interpolation_description interpolation_list[INTERPOLATION_TYPES_COUNT];

extern color_rgb blackColor;
extern color_rgb transparancyColor;
extern color_rgb bgColor;
extern color_rgb fgColor;

coordinates confineCoordinatesToRectangle(int x, int y, rectangle restriction_rectangle);

unsigned long rgbToHex(color_rgb color);
color_rgb hexToRgb(unsigned long color_hex);

int isRGBColorsEqual(color_rgb color_1, color_rgb color_2);
color_rgb getNotMaskPixelColor(color_rgb pixel_color);
color_rgb getFgmaskPixelColor(color_rgb pixel_color);
color_rgb getBgmaskPixelColor(color_rgb pixel_color);

color_rgb pixelBilinearInterpolation(float fractionOfX, float fractionOfY, color_rgb p11, color_rgb p12, color_rgb p21, color_rgb p22);

double lanczosKernel(double x, int lanczos_a);

#endif //SRC_GRAPHICS_COMMON_H
