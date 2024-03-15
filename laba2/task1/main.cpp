#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <omp.h>
#include <malloc.h>

using namespace std;

double CpuTimeSecond() {
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return ((double)ts.tv_sec + (double)ts.tv_nsec * 1.e-9);
}

void matrixmult(double* a, double* b, double* c, int m, int n) {
    for (int i = 0; i < m; i++) {
        c[i] = 0.0;
        for (int j = 0; j < n; j++)
            c[i] += a[i * n + j] * b[j];
    }
}

void parallmatrixmult(double* a, double* b, double* c, int m, int n, int numThread) {
#pragma omp parallel num_threads(numThread)
    {
        int nthreads = omp_get_num_threads();
        int threadid = omp_get_thread_num();
        int items_per_thread = m / nthreads;
        int lb = threadid * items_per_thread;
        int ub = (threadid == nthreads - 1) ? (m - 1) : (lb + items_per_thread - 1);
        for (int i = lb; i <= ub; i++) {
            c[i] = 0.0;
            for (int j = 0; j < n; j++)
                c[i] += a[i * n + j] * b[j];
        }
    }
}

//#ifdef PARALLEL
void run_parallel(size_t n, size_t m, int numThread) {
    double* a, * b, * c;

    a = (double*)malloc(sizeof(double) * m * n);
    b = (double*)malloc(sizeof(double) * n);
    c = (double*)malloc(sizeof(double) * m);

    double t = CpuTimeSecond();
#pragma omp parallel num_threads(numThread)
    {
        int nthreads = omp_get_num_threads();
        int threadid = omp_get_thread_num();
        int items_per_thread = m / nthreads;
        int lb = threadid * items_per_thread;
        int ub = (threadid == nthreads - 1) ? (m - 1) : (lb + items_per_thread - 1);
        for (int i = lb; i <= ub; i++) {
            for (int j = 0; j < n; j++)
                a[i * n + j] = i + j;
            c[i] = 0.0;
        }
    }
    for (int j = 0; j < n; j++)
        b[j] = j;
    parallmatrixmult(a, b, c, m, n, numThread);
    t = CpuTimeSecond() - t;

    printf("Elapsed time (parallel): %.6f sec.\n", t);
    free(a);
    free(b);
    free(c);
}
//#else
void run_serial(size_t n, size_t m) {
    double* a, * b, * c;
    a = (double*)malloc(sizeof(double) * m * n);
    b = (double*)malloc(sizeof(double) * n);
    c = (double*)malloc(sizeof(double) * m);

    double t = CpuTimeSecond();
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < n; j++)
            a[i * n + j] = i + j;
    }

    for (int j = 0; j < n; j++)
        b[j] = j;

    matrixmult(a, b, c, m, n);
    t = CpuTimeSecond() - t;

    printf("Elapsed time (serial): %.6f sec.\n", t);
    free(a);
    free(b);
    free(c);
}
//#endif

int main(int argc, char* argv[]) {
    size_t m = 4000;
    size_t n = 4000;
    int numThread = 6;

    if (argc > 1)
        numThread = atoi(argv[1]);
    if (argc > 2)
        m = atoi(argv[2]);
    if (argc > 3)
        n = atoi(argv[3]);

//#ifdef PARALLEL
    run_parallel(n, m, numThread);
//#else
    run_serial(n, m);
//#endif

    return 0;
}
