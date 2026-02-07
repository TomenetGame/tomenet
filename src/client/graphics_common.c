//
// Created by ekomov on 02.04.25.
//
#include "angband.h"
#include "graphics_common.h"
#include "math.h"
#include <X11/Xutil.h>

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

unsigned long rgbToHex(uint8_t red, uint8_t green, uint8_t blue) {
    return ((unsigned long)red << 16) | ((unsigned long)green << 8) | blue;
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

color_rgb get_rgb_from_pixel(Display *display, unsigned long pixel) {
    color_rgb rgb = blackColor; // Initialize to black
    XColor color;

    color.pixel = pixel;
    XQueryColor(display, DefaultColormapOfScreen(DefaultScreenOfDisplay(display)), &color);

    // XQueryColor returns values in the range 0-65535.  Scale to 0-255.
    rgb.red   = (uint8_t)(color.red   / 256);
    rgb.green = (uint8_t)(color.green / 256);
    rgb.blue  = (uint8_t)(color.blue  / 256);

    return rgb;
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


int save_ximage_as_bmp(XImage *image, const char *filename)  // TODO remove
 {
    FILE *fp;

    if (!image) {
        fprintf(stderr, "Ошибка: XImage пуст.\n");
        return -1;
    }

    // --- Шаг 1: Проверка и подготовка ---

    // BMP требует, чтобы данные были выровнены по 4 байта на строку.
    // XImage может использовать выравнивание, отличное от 4 байт.
    // Мы рассчитаем необходимый размер строки с учетом выравнивания (padding).
    int bits_per_pixel = 24; // BMP по умолчанию для XImage 24-бит

    // Размер строки (в байтах), округленный до ближайшего кратного 4
    int row_size_bytes = ((image->width * bits_per_pixel + 31) / 32) * 4;

    unsigned int data_size = row_size_bytes * image->height;
    unsigned int file_size = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader) + data_size;

    // XImage возвращает данные в формате (R, G, B, A) или (B, G, R, A),
    // а BMP хранит их как (B, G, R) без альфа-канала.

    // --- Шаг 2: Создание и запись заголовков ---

    BMPFileHeader file_header = {0};
    file_header.file_type = 0x4D42; // 'BM'
    file_header.file_size = file_size;
    file_header.offset_data = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader);

    BMPInfoHeader info_header = {0};
    info_header.header_size = 40;
    info_header.width = image->width;
    info_header.height = -image->height; // Высота отрицательная, чтобы данные шли сверху вниз (стандарт BMP)
    info_header.planes = 1;
    info_header.bits_per_pixel = bits_per_pixel;
    info_header.compression = 0; // BI_RGB
    info_header.image_size = data_size;

    fp = fopen(filename, "wb");
    if (!fp) {
        perror("Не удалось открыть файл для записи");
        return -1;
    }

    // Запись заголовков
    fwrite(&file_header, sizeof(BMPFileHeader), 1, fp);
    fwrite(&info_header, sizeof(BMPInfoHeader), 1, fp);

    // --- Шаг 3: Запись данных пикселей ---

    // ВНИМАНИЕ: XImage.data возвращает данные, специфичные для текущего
    // визуального представления (Visual). Для прямого сохранения
    // нам нужно преобразовать эти данные в 24-битный RGB, который понимает BMP.

    // Здесь мы делаем ПРЕДПОЛОЖЕНИЕ, что XImage возвращает данные,
    // которые можно переупорядочить в 24-битный формат.
    // Для большинства случаев, где XImage был получен через XGetImage,
    // это требует сложного маппинга (image->red_mask, image->blue_mask и т.д.).

    // *** Для упрощения, этот пример предполагает, что XImage уже находится
    // *** в формате, который можно скопировать, или мы работаем с 8-битной
    // *** глубиной, которую затем масштабируем до 24-бит. ***

    // Если XImage имеет depth=24 и правильные маски, можно использовать:

    unsigned char *bmp_row_data = (unsigned char*)malloc(row_size_bytes);

    for (int y = 0; y < image->height; y++) {
        memset(bmp_row_data, 0, row_size_bytes); // Заполняем строку нулями (для padding)

        for (int x = 0; x < image->width; x++) {
            // Получаем пиксель из XImage
            unsigned long pixel = XGetPixel(image, x, y);

            // *** КРИТИЧЕСКИЙ ШАГ: Преобразование Xlib Pixel в BGR ***
            // Здесь нужна функция для преобразования 'pixel' в B, G, R на основе image->red_mask, image->green_mask и т.д.

            unsigned char r, g, b;

            // **ПРИМЕР ПРЕОБРАЗОВАНИЯ (Может не работать для всех Visual!)**
            // Это очень грубое предположение для 24-битного прямого цвета:
            if (image->depth == 24) {
                // Если маски известны, нужно использовать XGetRed/Green/BlueData
                // Но в общем случае проще использовать XGetPixel и маски:
                r = (pixel & image->red_mask) >> ffs(image->red_mask) - 1;
                g = (pixel & image->green_mask) >> ffs(image->green_mask) - 1;
                b = (pixel & image->blue_mask) >> ffs(image->blue_mask) - 1;
            } else {
                 // Для других глубин (например, 8 бит) нужен доступ к цветовой карте XAllocColorCells
                 fprintf(stderr, "Внимание: Глубина (%d) не 24. Требуется сложная конвертация палитры.\n", image->depth);
                 // В случае ошибки конвертации, просто продолжаем, но данные будут некорректны
                 r = g = b = 0;
            }

            // BMP хранит: B, G, R
            int offset = x * 3;
            bmp_row_data[offset + 0] = b; // Blue
            bmp_row_data[offset + 1] = g; // Green
            bmp_row_data[offset + 2] = r; // Red
        }

        // Записываем готовую строку (включая padding)
        fwrite(bmp_row_data, 1, row_size_bytes, fp);
    }

    free(bmp_row_data);
    fclose(fp);
    printf("Изображение успешно сохранено в %s\n", filename);
    return 0;
}