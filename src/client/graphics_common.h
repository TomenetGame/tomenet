//
// Created by ekomov on 02.04.25.
//

#ifndef SRC_GRAPHICS_COMMON_H
#define SRC_GRAPHICS_COMMON_H

#include <X11/Xlib.h>

#define INTERPOLATION_NEAR 0
#define INTERPOLATION_LINEAR 1
#define INTERPOLATION_LANCZOS 2
#define INTERPOLATION_QUADRATIC 3

#define LANCZOS_A 1
#define LANCZOS_SAMPLE_LENGTH (LANCZOS_A * 2 + 1)

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

typedef struct
{
    float left;
    float right;
} linear_sample;

typedef struct
{
    linear_sample top;
    linear_sample bottom;
} bilinear_sample;

typedef struct
{
    double data[LANCZOS_SAMPLE_LENGTH][LANCZOS_SAMPLE_LENGTH];
} lanczos_sample_2d;

extern color_rgb blackColor;
extern color_rgb transparancyColor;
extern color_rgb bgColor;
extern color_rgb fgColor;

coordinates confineCoordinatesToRectangle(int x, int y, rectangle restriction_rectangle);

unsigned long rgbToHex(uint8_t red, uint8_t green, uint8_t blue);
color_rgb hexToRgb(unsigned long color_hex);

int isRGBColorsEqual(color_rgb color_1, color_rgb color_2);
color_rgb getNotMaskPixelColor(color_rgb pixel_color);
color_rgb getFgmaskPixelColor(color_rgb pixel_color);
color_rgb getBgmaskPixelColor(color_rgb pixel_color);

double quadraticInterpolation(double x, double y0, double y1, double y2);
color_rgb pixelBilinearInterpolation(float fractionOfX, float fractionOfY, color_rgb p11, color_rgb p12, color_rgb p21, color_rgb p22);

double lanczosResample(lanczos_sample_2d sample, double target_x, double target_y);
double lanczosKernel(double x, int lanczos_a);

// --- Структуры для заголовков BMP-файла ---
// Заголовок файла (14 байт)
#pragma pack(push, 1) // Устанавливаем выравнивание в 1 байт
typedef struct {
    unsigned short file_type;    // 'BM' (0x4D42)
    unsigned int file_size;      // Размер всего файла
    unsigned short reserved1;    // 0
    unsigned short reserved2;    // 0
    unsigned int offset_data;    // Смещение данных пикселей
} BMPFileHeader;

// Заголовок информации (40 байт, BITMAPINFOHEADER)
typedef struct {
    unsigned int header_size;    // Размер этого заголовка (40)
    int width;
    int height;
    unsigned short planes;       // 1
    unsigned short bits_per_pixel; // 24 для True Color
    unsigned int compression;    // 0 (BI_RGB, без сжатия)
    unsigned int image_size;     // Размер данных пикселей
    int x_pixels_per_meter;      // 0
    int y_pixels_per_meter;      // 0
    unsigned int colors_used;    // 0
    unsigned int important_colors; // 0
} BMPInfoHeader;
#pragma pack(pop) // Восстанавливаем стандартное выравнивание


/**
 * @brief Сохраняет XImage в файл формата BMP.
 *
 * Этот пример оптимизирован для 24-битных (True Color) изображений.
 * Если XImage имеет другую глубину (например, 8-битную),
 * требуется сложная логика для преобразования цветов (палитры).
 *
 * @param image Указатель на XImage.
 * @param filename Имя файла BMP для сохранения.
 * @return 0 при успехе, -1 при ошибке.
 */
int save_ximage_as_bmp(XImage *image, const char *filename); // TODO remove




#endif //SRC_GRAPHICS_COMMON_H