#ifndef TIMER_H
#define TIMER_H

typedef struct {
    double total_time;       /* Tiempo de pared total (I/O + computo + sync) */
    double io_time;          /* Tiempo dedicado a lectura y guardado en disco */
    double compute_time;     /* Tiempo puro de ejecucion de filtros */
    double sync_time;        /* Tiempo de barreras y recoleccion */
    int num_threads;         /* Hilos utilizados */
    int items_processed;     /* Cantidad de imagenes procesadas */
} BenchmarkResult;

double get_time_sec(void);

#endif /* TIMER_H */
