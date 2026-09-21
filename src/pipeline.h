#ifndef PIPELINE_H
#define PIPELINE_H

#include "timer.h"

typedef enum {
    MODE_BATCH,  /* Paralelizacion a nivel de lote de archivos */
    MODE_PIXEL   /* Paralelizacion a nivel de matriz de pixeles */
} ExecutionMode;

/**
 * Procesa una unica imagen aplicando Grayscale -> Gaussian Blur -> Sobel.
 * Si mode == MODE_PIXEL, la matriz de pixeles se divide entre num_threads.
 * Si mode == MODE_BATCH, se ejecuta con 1 hilo a nivel de pixel (pensado para uso en bucle).
 */
BenchmarkResult process_image_file(const char *input_path, const char *output_path,
                                   int num_threads, ExecutionMode mode);

/**
 * Procesa un directorio completo de imagenes.
 * Si mode == MODE_BATCH: reparte los archivos del lote entre num_threads.
 * Si mode == MODE_PIXEL: procesa imagen por imagen dividiendo cada matriz de pixeles entre num_threads.
 */
BenchmarkResult process_directory(const char *input_dir, const char *output_dir,
                                  int num_threads, ExecutionMode mode);

#endif /* PIPELINE_H */
