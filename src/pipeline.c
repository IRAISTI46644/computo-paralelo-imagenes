#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L

#define STB_IMAGE_IMPLEMENTATION
#include "vendor/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "vendor/stb_image_write.h"

#include "pipeline.h"
#include "filters.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <omp.h>

static int is_image_file(const char *filename) {
    const char *ext = strrchr(filename, '.');
    if (!ext) return 0;
    return (strcasecmp(ext, ".png") == 0 ||
            strcasecmp(ext, ".jpg") == 0 ||
            strcasecmp(ext, ".jpeg") == 0 ||
            strcasecmp(ext, ".bmp") == 0);
}

static void ensure_dir_exists(const char *path) {
    struct stat st = {0};
    if (stat(path, &st) == -1) {
#ifdef _WIN32
        mkdir(path);
#else
        mkdir(path, 0755);
#endif
    }
}

BenchmarkResult process_image_file(const char *input_path, const char *output_path,
                                   int num_threads, ExecutionMode mode) {
    BenchmarkResult res = {0};
    res.num_threads = num_threads;
    res.items_processed = 1;

    double t_start = get_time_sec();

    /* 1. Carga de imagen (I/O) */
    double t_io0 = get_time_sec();
    int width, height, channels;
    unsigned char *img_data = stbi_load(input_path, &width, &height, &channels, 0);
    double t_io1 = get_time_sec();
    res.io_time += (t_io1 - t_io0);

    if (!img_data) {
        fprintf(stderr, "Error: No se pudo cargar la imagen '%s': %s\n",
                input_path, stbi_failure_reason());
        return res;
    }

    /* 2. Procesamiento de filtros */
    size_t out_size = (size_t)width * height;
    unsigned char *out_pixels = (unsigned char *)malloc(out_size);
    if (!out_pixels) {
        fprintf(stderr, "Error: Memoria insuficiente para la salida.\n");
        stbi_image_free(img_data);
        return res;
    }

    int threads_for_pixel = (mode == MODE_PIXEL) ? num_threads : 1;
    double comp_time = 0.0;
    process_image_pipeline(img_data, width, height, channels, out_pixels,
                           threads_for_pixel, &comp_time);
    res.compute_time = comp_time;

    /* 3. Guardado en disco (I/O) */
    double t_io2 = get_time_sec();
    if (!stbi_write_png(output_path, width, height, 1, out_pixels, width)) {
        fprintf(stderr, "Error al escribir la imagen de salida '%s'\n", output_path);
    }
    double t_io3 = get_time_sec();
    res.io_time += (t_io3 - t_io2);

    free(out_pixels);
    stbi_image_free(img_data);

    double t_end = get_time_sec();
    res.total_time = t_end - t_start;
    res.sync_time = res.total_time - (res.compute_time + res.io_time);
    if (res.sync_time < 0.0) res.sync_time = 0.0;

    return res;
}

BenchmarkResult process_directory(const char *input_dir, const char *output_dir,
                                  int num_threads, ExecutionMode mode) {
    BenchmarkResult res = {0};
    res.num_threads = num_threads;

    ensure_dir_exists(output_dir);

    DIR *d = opendir(input_dir);
    if (!d) {
        fprintf(stderr, "Error: No se pudo abrir el directorio de entrada '%s'\n", input_dir);
        return res;
    }

    /* Recolectar lista de archivos compatibles */
    char **file_list = NULL;
    int file_count = 0;
    int capacity = 0;

    struct dirent *dir;
    while ((dir = readdir(d)) != NULL) {
        /* Ignorar . y .. */
        if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0) {
            continue;
        }

        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", input_dir, dir->d_name);
        struct stat st;
        if (stat(full_path, &st) == 0 && S_ISREG(st.st_mode)) {
            if (is_image_file(dir->d_name)) {
                if (file_count >= capacity) {
                    capacity = (capacity == 0) ? 64 : capacity * 2;
                    file_list = (char **)realloc(file_list, capacity * sizeof(char *));
                }
                file_list[file_count] = strdup(dir->d_name);
                file_count++;
            }
        }
    }
    closedir(d);

    if (file_count == 0) {
        fprintf(stderr, "Aviso: No se encontraron imagenes validas en '%s'\n", input_dir);
        free(file_list);
        return res;
    }

    res.items_processed = file_count;
    double t_global_start = get_time_sec();

    if (mode == MODE_BATCH) {
        /* MODO BATCH: Repartir el lote de archivos entre los hilos OpenMP */
        double total_compute_acc = 0.0;
        double total_io_acc = 0.0;

        #pragma omp parallel num_threads(num_threads)
        {
            double local_compute = 0.0;
            double local_io = 0.0;

            #pragma omp for schedule(dynamic, 1)
            for (int i = 0; i < file_count; i++) {
                char in_path[1024];
                char out_path[1024];
                snprintf(in_path, sizeof(in_path), "%s/%s", input_dir, file_list[i]);
                snprintf(out_path, sizeof(out_path), "%s/proc_%s", output_dir, file_list[i]);

                char *ext = strrchr(out_path, '.');
                if (ext) strcpy(ext, ".png");

                double t_read0 = get_time_sec();
                int w, h, ch;
                unsigned char *data = stbi_load(in_path, &w, &h, &ch, 0);
                double t_read1 = get_time_sec();
                local_io += (t_read1 - t_read0);

                if (data) {
                    unsigned char *out = (unsigned char *)malloc((size_t)w * h);
                    if (out) {
                        double c_time = 0.0;
                        process_image_pipeline(data, w, h, ch, out, 1, &c_time);
                        local_compute += c_time;

                        double t_write0 = get_time_sec();
                        stbi_write_png(out_path, w, h, 1, out, w);
                        double t_write1 = get_time_sec();
                        local_io += (t_write1 - t_write0);

                        free(out);
                    }
                    stbi_image_free(data);
                }
            }

            #pragma omp critical
            {
                total_compute_acc += local_compute;
                total_io_acc += local_io;
            }
        }

        double t_global_end = get_time_sec();
        res.total_time = t_global_end - t_global_start;
        res.compute_time = total_compute_acc / (double)num_threads;
        res.io_time = total_io_acc / (double)num_threads;
        res.sync_time = res.total_time - (res.compute_time + res.io_time);
        if (res.sync_time < 0.0) res.sync_time = 0.0;

    } else {
        /* MODO PIXEL: Procesar una a una, paralelizando la matriz interna con OpenMP */
        for (int i = 0; i < file_count; i++) {
            char in_path[1024];
            char out_path[1024];
            snprintf(in_path, sizeof(in_path), "%s/%s", input_dir, file_list[i]);
            snprintf(out_path, sizeof(out_path), "%s/proc_%s", output_dir, file_list[i]);

            char *ext = strrchr(out_path, '.');
            if (ext) strcpy(ext, ".png");

            BenchmarkResult single = process_image_file(in_path, out_path, num_threads, MODE_PIXEL);
            res.compute_time += single.compute_time;
            res.io_time += single.io_time;
            res.sync_time += single.sync_time;
        }
        double t_global_end = get_time_sec();
        res.total_time = t_global_end - t_global_start;
    }

    for (int i = 0; i < file_count; i++) {
        free(file_list[i]);
    }
    free(file_list);

    return res;
}
