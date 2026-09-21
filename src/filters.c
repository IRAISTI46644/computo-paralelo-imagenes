#include "filters.h"
#include <stdlib.h>
#include <math.h>
#include <omp.h>

/* Kernel Gaussiano 5x5 estandar (Suma = 273) */
static const int GAUSS_KERNEL[5][5] = {
    { 1,  4,  7,  4, 1 },
    { 4, 16, 26, 16, 4 },
    { 7, 26, 41, 26, 7 },
    { 4, 16, 26, 16, 4 },
    { 1,  4,  7,  4, 1 }
};
static const int GAUSS_SUM = 273;

/* Kernels de Sobel para derivadas direccionales en X e Y */
static const int SOBEL_X[3][3] = {
    { -1, 0, 1 },
    { -2, 0, 2 },
    { -1, 0, 1 }
};

static const int SOBEL_Y[3][3] = {
    { -1, -2, -1 },
    {  0,  0,  0 },
    {  1,  2,  1 }
};

static inline int clamp(int val, int min_val, int max_val) {
    if (val < min_val) return min_val;
    if (val > max_val) return max_val;
    return val;
}

void apply_grayscale(const unsigned char *src, unsigned char *dst,
                     int width, int height, int channels, int num_threads) {
    if (channels == 1) {
        #pragma omp parallel for num_threads(num_threads) schedule(static)
        for (int i = 0; i < width * height; i++) {
            dst[i] = src[i];
        }
        return;
    }

    #pragma omp parallel for num_threads(num_threads) schedule(static)
    for (int i = 0; i < width * height; i++) {
        int idx = i * channels;
        unsigned char r = src[idx];
        unsigned char g = src[idx + 1];
        unsigned char b = src[idx + 2];
        /* Formula de luminancia ITU-R BT.601 */
        int gray = (int)(0.299f * r + 0.587f * g + 0.114f * b + 0.5f);
        dst[i] = (unsigned char)clamp(gray, 0, 255);
    }
}

void apply_gaussian_blur(const unsigned char *src, unsigned char *dst,
                         int width, int height, int num_threads) {
    #pragma omp parallel for num_threads(num_threads) schedule(static)
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int acc = 0;
            for (int ky = -2; ky <= 2; ky++) {
                int py = clamp(y + ky, 0, height - 1);
                int row_offset = py * width;
                for (int kx = -2; kx <= 2; kx++) {
                    int px = clamp(x + kx, 0, width - 1);
                    acc += src[row_offset + px] * GAUSS_KERNEL[ky + 2][kx + 2];
                }
            }
            dst[y * width + x] = (unsigned char)(acc / GAUSS_SUM);
        }
    }
}

void apply_sobel(const unsigned char *src, unsigned char *dst,
                 int width, int height, int num_threads) {
    #pragma omp parallel for num_threads(num_threads) schedule(static)
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int gx = 0;
            int gy = 0;

            for (int ky = -1; ky <= 1; ky++) {
                int py = clamp(y + ky, 0, height - 1);
                int row_offset = py * width;
                for (int kx = -1; kx <= 1; kx++) {
                    int px = clamp(x + kx, 0, width - 1);
                    unsigned char val = src[row_offset + px];
                    gx += val * SOBEL_X[ky + 1][kx + 1];
                    gy += val * SOBEL_Y[ky + 1][kx + 1];
                }
            }

            /* Magnitud del gradiente aproximada o euclidiana */
            int magnitude = (int)sqrtf((float)(gx * gx + gy * gy));
            dst[y * width + x] = (unsigned char)clamp(magnitude, 0, 255);
        }
    }
}

void process_image_pipeline(const unsigned char *src_pixels,
                            int width, int height, int channels,
                            unsigned char *out_sobel,
                            int num_threads,
                            double *out_compute_time) {
    size_t num_pixels = (size_t)width * height;
    unsigned char *gray = (unsigned char *)malloc(num_pixels);
    unsigned char *blur = (unsigned char *)malloc(num_pixels);

    if (!gray || !blur) {
        free(gray);
        free(blur);
        return;
    }

    double t0 = omp_get_wtime();

    /* 1. Escala de grises */
    apply_grayscale(src_pixels, gray, width, height, channels, num_threads);

    /* 2. Desenfoque Gaussiano 5x5 */
    apply_gaussian_blur(gray, blur, width, height, num_threads);

    /* 3. Deteccion de bordes con Sobel */
    apply_sobel(blur, out_sobel, width, height, num_threads);

    double t1 = omp_get_wtime();

    if (out_compute_time) {
        *out_compute_time = t1 - t0;
    }

    free(gray);
    free(blur);
}
