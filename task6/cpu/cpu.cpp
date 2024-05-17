#include <iostream>
#include <boost/program_options.hpp>
#include <cmath>
#include <memory>
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <chrono>
namespace opt = boost::program_options;

double linearInterpolation(double x, double x1, double y1, double x2, double y2) {
    return y1 + ((x - x1) * (y2 - y1) / (x2 - x1));
}


void saveMatrixToFile(const std::unique_ptr<double[]>& matrix, int N, const std::string& filename) {
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


void initMatrix(std::unique_ptr<double[]>& arr, int N) {

    arr[0] = 10.0;
    arr[N - 1] = 20.0;
    arr[(N - 1) * N + (N - 1)] = 30.0;
    arr[(N - 1) * N] = 20.0;

    for (size_t i = 1; i < N - 1; i++)
    {
        //код инициализирует только элементы на границах матрицы , остальные элементы остаются неопределенными.
        arr[0 * N + i] = linearInterpolation(i, 0.0, arr[0], N - 1, arr[N - 1]);
        arr[i * N + 0] = linearInterpolation(i, 0.0, arr[0], N - 1, arr[(N - 1) * N]);
        arr[i * N + (N - 1)] = linearInterpolation(i, 0.0, arr[N - 1], N - 1, arr[(N - 1) * N + (N - 1)]);
        arr[(N - 1) * N + i] = linearInterpolation(i, 0.0, arr[(N - 1) * N], N - 1, arr[(N - 1) * N + (N - 1)]);
    }
}
int main(int argc, char const* argv[])
{
    opt::options_description desc("опции");
    desc.add_options()
        ("accuracy", opt::value<double>()->default_value(1e-6), "точность")
        ("cellsCount", opt::value<int>()->default_value(256), "размер матрицы")
        ("iterCount", opt::value<int>()->default_value(50), "количество операций")
        ("help", "помощь")
        ;

    opt::variables_map vm;

    opt::store(opt::parse_command_line(argc, argv, desc), vm); //анализирует аргументы командной строки(argc и argv)

    opt::notify(vm);//проверяет все ли аргументы были переданы

    if (vm.count("help")) {
        std::cout << desc << "\n";
        return 1;
    }

    int N = vm["cellsCount"].as<int>();
    double accuracy = vm["accuracy"].as<double>();
    int countIter = vm["iterCount"].as<int>();

    double error = 1.0;
    int iter = 0;

    std::unique_ptr<double[]> A(new double[N * N]);
    std::unique_ptr<double[]> Anew(new double[N * N]);
    initMatrix(A, N);
    initMatrix(Anew, N);
    double* prevmatrix = Anew.get();
    double* curmatrix = A.get();


    auto start = std::chrono::high_resolution_clock::now();
    while (iter < countIter && iter<10000000 && error > accuracy) {
#pragma acc parallel loop independent collapse(2) gang num_gangs(40)//independent-итерации цикла независимы друг от друга; collapse(2)-для объединения двух вложенных циклов в один параллельный цикл;gang num_gangs(40 - цикл будет разделен на 40 частей, и каждая часть будет выполняться одновременно на разных потоках устройства.
        for (size_t i = 1; i < N - 1; i++)
        {
            for (size_t j = 1; j < N - 1; j++)
            {
                curmatrix[i * N + j] = 0.25 * (prevmatrix[i * N + j + 1] + prevmatrix[i * N + j - 1] + prevmatrix[(i - 1) * N + j] + prevmatrix[(i + 1) * N + j]);
            }
        }

        if ((iter + 1) % 1000 == 0) {
            error = 0.0;
#pragma acc parallel loop independent collapse(2) gang num_gangs(40) reduction(max:error)//reduction(max:error) означает, что будет вычисляться максимальное значение среди всех значений переменной error
            for (size_t i = 1; i < N - 1; i++)
            {
                for (size_t j = 1; j < N - 1; j++)
                {
                    error = fmax(error, fabs(curmatrix[i * N + j] - prevmatrix[i * N + j]));
                }
            }
            std::cout << "iteration: " << iter + 1 << ' ' << "error: " << error << std::endl;

        }

        double* temp = prevmatrix;
        prevmatrix = curmatrix;
        curmatrix = temp;

        iter++;
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto time_s = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    std::cout << "time: " << time_s << " error: " << error << " iterarion: " << iter << std::endl;
    if (N <= 13) {

        for (size_t i = 0; i < N; i++)
        {
            for (size_t j = 0; j < N; j++)
            {
                std::cout << A[i * N + j] << ' ';
            }
            std::cout << std::endl;
        }
    }
    saveMatrixToFile(std::ref(A), N, "matrix.txt");
    A = nullptr;
    Anew = nullptr;
    return 0;
}
