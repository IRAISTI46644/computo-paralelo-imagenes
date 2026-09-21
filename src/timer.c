#include "timer.h"
#include <omp.h>

double get_time_sec(void) {
    return omp_get_wtime();
}
