#include <iostream>
#include <boost/program_options.hpp>
#include <cmath>
#include <memory>
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <chrono>
#include "cublas_v2.h"

namespace opt = boost::program_options;

double linearInterpolation(double x, double x1, double y1, double x2, double y2) {
    return y1 + ((x - x1) * (y2 - y1) / (x2 - x1));
}

void initMatrix(std::vector<double>& arr, int N) {
    arr[0] = 10.0;
    arr[N - 1] = 20.0;
    arr[(N - 1) * N + (N - 1)] = 30.0;
    arr[(N - 1) * N] = 20.0;

    for (size_t i = 1; i < N - 1; i++) {
        arr[0 * N + i] = linearInterpolation(i, 0.0, arr[0], N - 1, arr[N - 1]);
        arr[i * N + 0] = linearInterpolation(i, 0.0, arr[0], N - 1, arr[(N - 1) * N]);
        arr[i * N + (N - 1)] = linearInterpolation(i, 0.0, arr[N - 1], N - 1, arr[(N - 1) * N + (N - 1)]);
        arr[(N - 1) * N + i] = linearInterpolation(i, 0.0, arr[(N - 1) * N], N - 1, arr[(N - 1) * N + (N - 1)]);
    }
}

void saveMatrixToFile(const double* matrix, int N, const std::string& filename) {
    std::ofstream outputFile(filename);
    if (!outputFile.is_open()) {
        std::cerr << "Unable to open file " << filename << " for writing." << std::endl;
        return;
    }

    int fieldWidth = 10;

    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            outputFile << std::setw(fieldWidth) << std::fixed << std::setprecision(4) << matrix[i * N + j];
        }
        outputFile << std::endl;
    }

    outputFile.close();
}

int main(int argc, char const* argv[]) {
    opt::options_description desc("опции");
    desc.add_options()
        ("accuracy", opt::value<double>()->default_value(1e-6), "точность")
        ("cellsCount", opt::value<int>()->default_value(256), "размер матрицы")
        ("iterCount", opt::value<int>()->default_value(1000000), "число итераций ")
        ("help", "помощь")
        ;

    opt::variables_map vm;

    opt::store(opt::parse_command_line(argc, argv, desc), vm);

    opt::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << "\n";
        return 1;
    }

    int N = vm["cellsCount"].as<int>();
    double accuracy = vm["accuracy"].as<double>();
    int countIter = vm["iterCount"].as<int>();

    cublasStatus_t stat;//stat для хранения статуса функций
    cublasHandle_t handle;//указатель на структуру, содержащий информацию о состоянии и параметрах работы библиотеки CUBLAS.
    stat = cublasCreate(&handle);//инициализации cubla
    if (stat != CUBLAS_STATUS_SUCCESS) { // Проверяет успешность инициализации
        std::cout << "CUBLAS initialization failed\n";
        std::cout << "NAME: " << cublasGetStatusName(stat) << '\n';
        std::cout << "DES: " << cublasGetStatusString(stat) << '\n';//описание ошибки
        return 1;
    }
    else {
        std::cout << "CUBLAS initialization successful\n";
    }

    double error = 1.0;
    int iter = 0;

    std::vector<double> A(N * N);
    std::vector<double> Anew(N * N);

    initMatrix(A, N);
    initMatrix(Anew, N);

    auto start = std::chrono::high_resolution_clock::now();
    double* curmatrix = A.data();
    double* prevmatrix = Anew.data();

    double coef = -1.0;
    int idx = 0;

#pragma acc data copyin(idx,coef,prevmatrix[0:N*N],curmatrix[0:N*N],N)
    {
        while (iter < countIter && iter < 10000000 && error > accuracy) {
#pragma acc parallel loop independent collapse(2) present(curmatrix,prevmatrix)
            for (size_t i = 1; i < N - 1; i++) {
                for (size_t j = 1; j < N - 1; j++) {
                    curmatrix[i * N + j] = 0.25 * (prevmatrix[i * N + j + 1] + prevmatrix[i * N + j - 1] + prevmatrix[(i - 1) * N + j] + prevmatrix[(i + 1) * N + j]);
                }
            }

            if ((iter + 1) % 1000 == 0) {
#pragma acc data present(prevmatrix,curmatrix) wait
#pragma acc host_data use_device(curmatrix,prevmatrix)//будет использовать данные, находящиеся в памяти устройства
                {
                    stat = cublasDaxpy(handle, N * N, &coef, curmatrix, 1, prevmatrix, 1);// y : = α⋅xi+ yi α соответствует переменной coef,x соответствует массиву curmatrix,y соответствует массиву prevmatrix.
                    stat = cublasIdamax(handle, N * N, prevmatrix, 1, &idx);//cодержит  индекс элемента вектора prevmatrix с максимальным значением
                }
#pragma acc update self(prevmatrix[idx])
                error = fabs(prevmatrix[idx]);
                std::cout << "iteration: " << iter + 1 << ' ' << "error: " << error << std::endl;
#pragma acc host_data use_device(curmatrix,prevmatrix)
                {
                    stat = cublasDcopy(handle, N * N, curmatrix, 1, prevmatrix, 1);// копирует элементы вектора curmatrix в вектор prevmatrix
                }

            }
            std::swap(prevmatrix, curmatrix);
            iter++;
        }
        cublasDestroy(handle);
#pragma acc update self(curmatrix[0:N*N])
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;
    auto time_s = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    std::cout << "time: " << time_s << " error: " << error << " iteration: " << iter << std::endl;

    if (N <= 13) {
        for (size_t i = 0; i < N; i++) {
            for (size_t j = 0; j < N; j++) {
                std::cout << A[i * N + j] << ' ';
            }
            std::cout << std::endl;
        }
    }

    saveMatrixToFile(A.data(), N, "matrix2.txt");

    return 0;
}
