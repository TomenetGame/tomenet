//
// Created by ekomov on 02.04.25.
//

#ifndef SRC_GRAPHICS_COMMON_H
#define SRC_GRAPHICS_COMMON_H

#include <X11/Xlib.h>
#include <X11/Xutil.h>

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
    coordinates coordinates;
    color_rgb color;
} image_pixel;

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
    coordinates top_left;
    coordinates bottom_right;
} rectangle;

typedef struct
{
    double data[LANCZOS_SAMPLE_LENGTH][LANCZOS_SAMPLE_LENGTH];
} lanczos_sample_2d;

coordinates correctPixelCoordinates(int x, int y, rectangle restriction_rectangle);
color_rgb x_get_pixel_rgb(XImage *image, int x, int y);
void x_set_pixel_color(XImage *image, int x, int y, unsigned long pixel_color);
unsigned long rgb_to_hex(uint8_t red, uint8_t green, uint8_t blue);

// float linear_interpolation(float ratio, int value_left, int value_right);
float linear_interpolation(float ratio, linear_sample sample);

// float bilinear_interpolation(float ratio_x, float ratio_y, int value_11, int value_12, int value_21, int value_22);
float bilinear_interpolatqion(float ratio_x, float ratio_y, bilinear_sample sample);

// float quadratic_interpolation(float x, float x0, float f0, float x1, float f1, float x2, float f2);

color_rgb pixel_bilinear_interpolation(float fractionOfX, float fractionOfY, color_rgb p11, color_rgb p12, color_rgb p21, color_rgb p22);
color_rgb hex_to_rgb(unsigned long color_hex);
int isRGBColorsEqual(color_rgb color_1, color_rgb color_2);
color_rgb get_not_mask_pixel_color(color_rgb pixel_color);
color_rgb get_fg_mask_pixel_color(color_rgb pixel_color);
color_rgb get_bg_mask2_pixel_color(color_rgb pixel_color);
color_rgb get_rgb_from_pixel(Display *display, unsigned long pixel);

double quadratic_interpolation(double x, double y0, double y1, double y2);

double lanczos_resample(lanczos_sample_2d sample, double target_x, double target_y);
double lanczos_kernel(double x, int lanczos_a);



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
int save_ximage_as_bmp(XImage *image, const char *filename);




#endif //SRC_GRAPHICS_COMMON_H