#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/stat.h>
#include <omp.h>
#include "pipeline.h"

static void print_usage(const char *prog_name) {
    printf("===============================================================\n");
    printf("  SISTEMA PARALELO DE PROCESAMIENTO DE IMAGENES (OpenMP)\n");
    printf("  Proyecto Parcial 1 - Computo Paralelo y Distribuido\n");
    printf("===============================================================\n");
    printf("Uso:\n");
    printf("  %s --input <ruta> [opciones]\n\n", prog_name);
    printf("Opciones obligatorias:\n");
    printf("  -i, --input <ruta>       Archivo de imagen o directorio de entrada.\n\n");
    printf("Opciones generales:\n");
    printf("  -o, --output <ruta>      Ruta de archivo o carpeta de salida (def: ./data/output).\n");
    printf("  -t, --threads <N>        Numero de hilos OpenMP [1, 2, 4, 8, ...] (def: 1).\n");
    printf("  -m, --mode <batch|pixel> Estrategia de paralelismo (def: batch para dir, pixel para archivo).\n");
    printf("  -c, --csv                Salida en formato CSV (facil para scripts de benchmark).\n");
    printf("  -h, --help               Muestra esta ayuda.\n\n");
    printf("Ejemplos:\n");
    printf("  %s -i ./data/input/xray.jpg -o ./data/output/xray_proc.png -t 4\n", prog_name);
    printf("  %s -i ./data/input -o ./data/output -t 8 --mode batch\n", prog_name);
    printf("===============================================================\n");
}

static int is_directory(const char *path) {
    struct stat st;
    if (stat(path, &st) == 0) {
        return S_ISDIR(st.st_mode);
    }
    return 0;
}

int main(int argc, char *argv[]) {
    char *input_path = NULL;
    char *output_path = NULL;
    int num_threads = 1;
    char *mode_str = NULL;
    int csv_format = 0;

    static struct option long_options[] = {
        {"input",   required_argument, 0, 'i'},
        {"output",  required_argument, 0, 'o'},
        {"threads", required_argument, 0, 't'},
        {"mode",    required_argument, 0, 'm'},
        {"csv",     no_argument,       0, 'c'},
        {"help",    no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "i:o:t:m:ch", long_options, NULL)) != -1) {
        switch (opt) {
            case 'i': input_path = optarg; break;
            case 'o': output_path = optarg; break;
            case 't': num_threads = atoi(optarg); break;
            case 'm': mode_str = optarg; break;
            case 'c': csv_format = 1; break;
            case 'h': print_usage(argv[0]); return 0;
            default:  print_usage(argv[0]); return 1;
        }
    }

    if (!input_path) {
        fprintf(stderr, "Error: Se requiere especificar la ruta de entrada con -i o --input.\n");
        print_usage(argv[0]);
        return 1;
    }

    if (num_threads < 1) {
        num_threads = 1;
    }

    int input_is_dir = is_directory(input_path);

    /* Asignar salida por defecto si no se especifico */
    char default_out[512];
    if (!output_path) {
        if (input_is_dir) {
            strcpy(default_out, "./data/output");
        } else {
            strcpy(default_out, "./data/output/resultado_sobel.png");
        }
        output_path = default_out;
    }

    /* Determinar modo de ejecucion */
    ExecutionMode mode = MODE_BATCH;
    if (mode_str) {
        if (strcmp(mode_str, "pixel") == 0) {
            mode = MODE_PIXEL;
        } else if (strcmp(mode_str, "batch") == 0) {
            mode = MODE_BATCH;
        } else {
            fprintf(stderr, "Aviso: Modo '%s' no reconocido. Usando batch por defecto.\n", mode_str);
            mode = MODE_BATCH;
        }
    } else {
        mode = input_is_dir ? MODE_BATCH : MODE_PIXEL;
    }

    if (!csv_format) {
        printf("[INFO] Procesando entrada: %s (%s)\n", input_path, input_is_dir ? "Directorio" : "Archivo");
        printf("[INFO] Destino:           %s\n", output_path);
        printf("[INFO] Hilos OpenMP:      %d\n", num_threads);
        printf("[INFO] Modo:              %s\n", mode == MODE_BATCH ? "Lote (Batch)" : "Matriz de pixeles (Pixel)");
        printf("---------------------------------------------------------------\n");
    }

    BenchmarkResult res;
    if (input_is_dir) {
        res = process_directory(input_path, output_path, num_threads, mode);
    } else {
        res = process_image_file(input_path, output_path, num_threads, mode);
    }

    if (csv_format) {
        /* threads,items,total_time,compute_time,io_time,sync_time */
        printf("%d,%d,%.6f,%.6f,%.6f,%.6f\n",
               res.num_threads, res.items_processed,
               res.total_time, res.compute_time, res.io_time, res.sync_time);
    } else {
        printf("RESULTADOS DEL BENCHMARK:\n");
        printf("  - Imagenes procesadas:       %d\n", res.items_processed);
        printf("  - Hilos activos:             %d\n", res.num_threads);
        printf("  - Tiempo Total de ejecucion: %.6f s\n", res.total_time);
        printf("  - Tiempo Puro de Computo:    %.6f s\n", res.compute_time);
        printf("  - Tiempo de E/S (Disco):     %.6f s\n", res.io_time);
        printf("  - Overhead de Sincronizacion:%.6f s\n", res.sync_time);
        printf("===============================================================\n");
    }

    return 0;
}
