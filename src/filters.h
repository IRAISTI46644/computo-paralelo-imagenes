#ifndef FILTERS_H
#define FILTERS_H

#include <stddef.h>

/**
 * Convierte una imagen RGB/RGBA a escala de grises (1 canal) usando la fórmula de luminancia:
 * Y = 0.299*R + 0.587*G + 0.114*B
 *
 * @param src Puntero a los datos de la imagen original (RGB o RGBA).
 * @param dst Búfer de salida prealocado de tamaño width * height.
 * @param width Ancho en píxeles.
 * @param height Alto en píxeles.
 * @param channels Número de canales de la imagen fuente (1, 3 o 4).
 * @param num_threads Número de hilos OpenMP (1 = secuencial).
 */
void apply_grayscale(const unsigned char *src, unsigned char *dst,
                     int width, int height, int channels, int num_threads);

/**
 * Aplica un desenfoque gaussiano (Gaussian Blur) con kernel 5x5.
 *
 * @param src Imagen en escala de grises de entrada (width * height).
 * @param dst Búfer de salida prealocado de tamaño width * height.
 * @param width Ancho en píxeles.
 * @param height Alto en píxeles.
 * @param num_threads Número de hilos OpenMP (1 = secuencial).
 */
void apply_gaussian_blur(const unsigned char *src, unsigned char *dst,
                         int width, int height, int num_threads);

/**
 * Aplica el operador Sobel para detección de bordes (magnitud de gradiente Gx y Gy).
 *
 * @param src Imagen suavizada de entrada (width * height).
 * @param dst Búfer de salida prealocado de tamaño width * height.
 * @param width Ancho en píxeles.
 * @param height Alto en píxeles.
 * @param num_threads Número de hilos OpenMP (1 = secuencial).
 */
void apply_sobel(const unsigned char *src, unsigned char *dst,
                 int width, int height, int num_threads);

/**
 * Ejecuta el pipeline completo (Grayscale -> Gaussian Blur -> Sobel)
 * sobre una única imagen en memoria.
 *
 * @param src_pixels Píxeles de entrada cargados por stb_image.
 * @param width Ancho en píxeles.
 * @param height Alto en píxeles.
 * @param channels Canales originales.
 * @param out_sobel Búfer de salida final para la imagen con bordes detectados.
 * @param num_threads Número de hilos OpenMP.
 * @param out_compute_time Puntero opcional para registrar el tiempo puro de cálculo (sin I/O).
 */
void process_image_pipeline(const unsigned char *src_pixels,
                            int width, int height, int channels,
                            unsigned char *out_sobel,
                            int num_threads,
                            double *out_compute_time);

#endif /* FILTERS_H */
