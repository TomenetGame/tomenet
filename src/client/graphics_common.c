#include "angband.h"
#include "graphics_common.h"
#include "math.h"

color_rgb blackColor = {0, 0, 0};
color_rgb transparancyColor = {29, 33, 28};
color_rgb bgColor = {62, 61, 0};
color_rgb fgColor = {252, 0, 251};
// color_rgb fgColor = {251, 0, 252}; // hm...

float linear_interpolation(float ratio, linear_sample sample);
float bilinear_interpolation(float ratio_x, float ratio_y, bilinear_sample bilinear_sample);
bilinear_sample make_bilinear_sample(float topLeft, float topRight, float bottomLeft, float bottomRight);

coordinates confineCoordinatesToRectangle(int x, int y, rectangle restriction_rectangle) {
    coordinates correctedPixelCoordinates;
    correctedPixelCoordinates.x = x;
    correctedPixelCoordinates.y = y;

    if (x < restriction_rectangle.top_left.x) correctedPixelCoordinates.x = restriction_rectangle.top_left.x;
    if (x > restriction_rectangle.bottom_right.x) correctedPixelCoordinates.x = restriction_rectangle.bottom_right.x;

    if (y < restriction_rectangle.top_left.y) correctedPixelCoordinates.y = restriction_rectangle.top_left.y;
    if (y > restriction_rectangle.bottom_right.y) correctedPixelCoordinates.y = restriction_rectangle.bottom_right.y;

    return correctedPixelCoordinates;
}

unsigned long rgbToHex(color_rgb color) {
    return ((unsigned long)color.red << 16) | ((unsigned long)color.green << 8) | color.blue;
}

color_rgb hexToRgb(unsigned long color_hex) {
    color_rgb rgb;
    rgb.red   = (color_hex >> 16) & 0xFF;
    rgb.green = (color_hex >> 8) & 0xFF;
    rgb.blue  = color_hex & 0xFF;

    return rgb;
}

int isRGBColorsEqual(color_rgb color_1, color_rgb color_2)
{
    return (color_1.red == color_2.red) && (color_1.green == color_2.green) && (color_1.blue == color_2.blue);
}

color_rgb getNotMaskPixelColor(color_rgb pixel_color)
{
    if (isRGBColorsEqual(pixel_color, bgColor)
        || isRGBColorsEqual(pixel_color, transparancyColor)
        || isRGBColorsEqual(pixel_color, fgColor)
    ) {
        return blackColor;
    }

    return pixel_color;
}

color_rgb getFgmaskPixelColor(color_rgb pixel_color)
{
    if (isRGBColorsEqual(pixel_color, fgColor))
    {
        return pixel_color;
    }

    return blackColor;
}

color_rgb getBgmaskPixelColor(color_rgb pixel_color)
{
    if (isRGBColorsEqual(pixel_color, transparancyColor))
    {
        return pixel_color;
    }

    return blackColor;
}

double quadraticInterpolation(double x, double y0, double y1, double y2) {
    // Lagrange form of quadratic interpolation
    // double L0 = ((x - 1) * (x - 2)) / ((-1 - 1) * (-1 - 2));
    double L0 = ((x - 1) * (x - 2)) / ((0 - 1) * (0 - 2));
    double L1 = (x * (x - 2)) / ((1 - 0) * (1 - 2));
    double L2 = (x * (x - 1)) / ((2 - 0) * (2 - 1));

    return L0 * y0 + L1 * y1 + L2 * y2;
}

color_rgb pixelBilinearInterpolation(float fractionOfX, float fractionOfY, color_rgb p11, color_rgb p12, color_rgb p21, color_rgb p22)
{
    color_rgb new_color;

    bilinear_sample blue_sample = make_bilinear_sample(p11.blue, p12.blue, p21.blue, p22.blue);
    new_color.blue = (int) bilinear_interpolation(fractionOfX, fractionOfY, blue_sample);

    bilinear_sample green_sample = make_bilinear_sample(p11.green, p12.green, p21.green, p22.green);
    new_color.green = (int) bilinear_interpolation(fractionOfX, fractionOfY, green_sample);

    bilinear_sample red_sample = make_bilinear_sample(p11.red, p12.red, p21.red, p22.red);
    new_color.red = (int) bilinear_interpolation(fractionOfX, fractionOfY, red_sample);

    return new_color;
}

double lanczosResample(lanczos_sample_2d sample, double target_x, double target_y)
{
    double sum = 0.0;
    double weight_sum = 0.0;

    for (int i = 0; i < LANCZOS_SAMPLE_LENGTH; ++i)
    {
        for (int j = 0; j < LANCZOS_SAMPLE_LENGTH; ++j)
        {
            double dist_x = target_x - i; // probably need to adjust `-` sign
            double dist_y = target_y - j; // probably need to adjust `-` sign

            double weight = lanczosKernel(dist_x, LANCZOS_A) * lanczosKernel(dist_y, LANCZOS_A);

            sum += weight * sample.data[i][j];
            weight_sum += weight;
        }
    }

    return sum / weight_sum;
}


double lanczosKernel(double x, int lanczos_a) {
    x = fabs(x);
    if (x == 0.0) {
        return 1.0;
    } else if (fabs(x) < lanczos_a) {
        double pi_x = M_PI * x;
        return lanczos_a * sin(pi_x) * sin(pi_x / lanczos_a) / (pi_x * pi_x);
    } else {
        return 0.0;
    }
}

float linear_interpolation(float ratio, linear_sample sample)
{
    return (1 - ratio) * sample.left + ratio * sample.right;
}

float bilinear_interpolation(float ratio_x, float ratio_y, bilinear_sample bilinear_sample)
{
    linear_sample y_sample;

    y_sample.left = linear_interpolation(ratio_x, bilinear_sample.top);
    y_sample.right = linear_interpolation(ratio_x, bilinear_sample.bottom);

    return linear_interpolation(ratio_y, y_sample);
}

bilinear_sample make_bilinear_sample(float topLeft, float topRight, float bottomLeft, float bottomRight)
{
    bilinear_sample sample;
    linear_sample top_sample;
    linear_sample bottom_sample;

    top_sample.left = topLeft;
    top_sample.right = topRight;
    bottom_sample.left = bottomLeft;
    bottom_sample.right = bottomRight;

    sample.top = top_sample;
    sample.bottom = bottom_sample;

    return sample;
}

/*
float quadratic_interpolation(float x, float x0, float f0, float x1, float f1, float x2, float f2) {
    float L0, L1, L2;

    // Lagrange basis polynomials
    L0 = ((x - x1) * (x - x2)) / ((x0 - x1) * (x0 - x2));
    L1 = ((x - x0) * (x - x2)) / ((x1 - x0) * (x1 - x2));
    L2 = ((x - x0) * (x - x1)) / ((x2 - x0) * (x2 - x1));

    // Interpolated value
    return L0 * f0 + L1 * f1 + L2 * f2;
}

float bi_quadratic_interpolation(
        float x, float y,
        float x1, float x2,  float x3,
        float y1, float y2,  float y3,
        float f00, float f01, float f02,
        float f10, float f11, float f12,
        float f20, float f21, float f22
        ) {

    float F1, F2, F3;

    F1 = quadratic_interpolation(x, x1, f00, x2, f01, x3, f02);
    F2 = quadratic_interpolation(x, x1, f10, x2, f11, x3, f12);
    F3 = quadratic_interpolation(x, x1, f20, x2, f21, x3, f22);

    return quadratic_interpolation(y, y1, F1, y2, F2, y3, F3);
}

color_rgb pixel_bi_quadratic_interpolation(int x,  int y,
                                     color_rgb p00, color_rgb p01, color_rgb p02,
                                     color_rgb p10, color_rgb p11, color_rgb p12,
                                     color_rgb p20, color_rgb p21, color_rgb p22
                                     )
{
    color_rgb newColor = {0, 0, 0};

    return newColor;
}
*/


