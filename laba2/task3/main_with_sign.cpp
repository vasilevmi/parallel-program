
#include <iostream>
#include <vector>
#include <cmath>
#include <omp.h>
#include <fstream>
#include <cmath>
#include <memory>

using namespace std;

double eps = pow(10, -5);
int N = 5055;
double sumznam = 0.0;
double loss = 100.0;

void initializeSystem(int N, unique_ptr<unique_ptr<double[]>[]>& matr, unique_ptr<double[]>& b, unique_ptr<double[]>& x) {
    for (int i = 0; i < N; i++)
    {
        b[i] = N + 1;
        x[i] = 0;
        for (int j = 0; j < N; j++) {
            matr[i][j] = 1;
        }

    }
    double znam = 0.0;
    for (int i = 0; i < N; i++) {
        matr[i][i] = 2.0;
        znam += pow(b[i], 2);
    }
    sumznam = sqrt(znam);
}

double loss_prev = 9999999999;

unique_ptr<double[]> simpleIteration(unique_ptr<unique_ptr<double[]>[]>& matr, unique_ptr<double[]>& b, unique_ptr<double[]>& x, double tau) {
    unique_ptr<double[]> answ = make_unique<double[]>(N);
    double chisl = 0.0;
    double sumchisl = 0.0;
    while (loss > eps) {
#pragma omp parallel for
        for (int i = 0; i < N; i++) {
            double sum = 0.0;
            for (int j = 0; j < N; j++) {
                sum += matr[i][j] * x[j];
            }
            answ[i] = x[i] - tau * (sum - b[i]);
        }

        for (int i = 0; i < N; i++) {
            double smt = 0;
            for (int j = 0; j < N; j++) {
                smt += matr[i][j] * x[j];
            }
            chisl = pow((smt - b[i]), 2);
        }
        sumchisl = sqrt(chisl);
        loss = sumchisl / sumznam;
        printf("%f\n", loss);
        for (int i = 0; i < N; i++) x[i] = answ[i];
        chisl = 0;
    }
    return x;
}

unique_ptr<double[]> optim(unique_ptr<double[]>& x, int numThreads, unique_ptr<unique_ptr<double[]>[]>& matr, unique_ptr<double[]>& b, double tau)
{
    unique_ptr<double[]> answ1 = make_unique<double[]>(N);
    double sumer = 0;
#pragma omp parallel num_threads(numThreads)
    {
#pragma omp for schedule(dynamic, int(N / (numThreads * 3))) nowait reduction(+:sumer)
        for (int i = 0; i < N; i++) {
            answ1[i] = 0;
            for (int j = 0; j < N; j++) {
                answ1[i] += answ1[i] * matr[i][j] * x[j];
            }
            answ1[i] = answ1[i] - b[i];
            sumer += answ1[i] * answ1[i];
            answ1[i] = x[i] - tau * answ1[i];
        }
    }
    return answ1;
}

unique_ptr<double[]> matrix_omp_parallel(unique_ptr<double[]>& x, int numThreads, unique_ptr<unique_ptr<double[]>[]>& matr, unique_ptr<double[]>& b, double tau)
{
    unique_ptr<double[]> answ2 = make_unique<double[]>(N);
    double sum = 0;
#pragma omp parallel num_threads(numThreads)
    {
        double thread_sum = 0;
        int num_threads = omp_get_num_threads();
        int thread_id = omp_get_thread_num();
        int items_per_thread = N / num_threads;
        int lb = thread_id * items_per_thread;
        int ub = (thread_id == num_threads - 1) ? (N - 1) : (lb + items_per_thread - 1);

        for (int i = lb; i <= ub; i++) {
            answ2[i] = 0;
            for (int j = 0; j < N; j++) {
                answ2[i] += matr[i][j] * x[j];
            }
            answ2[i] = answ2[i] - b[i];
            thread_sum += answ2[i] * answ2[i];
            answ2[i] = x[i] - tau * answ2[i];
        }
#pragma omp atomic
        sum += thread_sum;
    }

    return answ2;
}

int main(int argc, char** argv) {
    double tau = 0.0001;
    int num_threads = 19;
    if (argc > 1)
        num_threads = std::atoi(argv[1]);
    unique_ptr<unique_ptr<double[]>[]> matr = make_unique<unique_ptr<double[]>[]>(N);
    for (int i = 0; i < N; i++)
        matr[i] = make_unique<double[]>(N);
    unique_ptr<double[]> x = make_unique<double[]>(N);
    unique_ptr<double[]> b = make_unique<double[]>(N);

    initializeSystem(N, matr, b, x);

    omp_set_num_threads(num_threads);

    double start_time = omp_get_wtime();
    x = simpleIteration(matr, b, x, tau);
    /*x = optim(x,num_threads,matr,b,tau);*/
    /*x = matrix_omp_parallel(x,num_threads,matr,b,tau);*/

    double end_time = omp_get_wtime();
    double duration = end_time - start_time;

    cout << "Time on " << num_threads << " threads: " << duration << " s" << endl;
    for (int j = 0; j < N; j++) {
        cout << x[j] << " ";
    }
    return 0;
}