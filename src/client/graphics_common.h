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

unsigned long rgbToHex(color_rgb color);
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
inline int save_ximage_as_bmp(XImage *image, const char *filename)  // TODO remove
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

#endif //SRC_GRAPHICS_COMMON_H
