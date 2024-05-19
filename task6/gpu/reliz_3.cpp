#include <iostream>
#include <boost/program_options.hpp>
#include <cmath>
#include <memory>
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <chrono>
namespace opt = boost::program_options;



// ñîáñòâåííî âîçâðàùàåò çíà÷åíèå ëèíåéíîé èíòåðïîëÿöèè
double linearInterpolation(double x, double x1, double y1, double x2, double y2) {
    // äåëàåì çíà÷åíèå y(ùíà÷åíèå êëåòêè)èñïîëüçóÿ ôîðìóëó ëèíåéíîé èíòåðïîëÿöèè
    return y1 + ((x - x1) * (y2 - y1) / (x2 - x1));
}




void initMatrix(std::unique_ptr<double[]>& arr, int N) {

    arr[0] = 10.0;
    arr[N - 1] = 20.0;
    arr[(N - 1) * N + (N - 1)] = 30.0;
    arr[(N - 1) * N] = 20.0;
    // èíèöèàëèçèðóåì è ïîòîì ñðàçó îòïðàâèì íà äåâàéñ
    for (size_t i = 1; i < N - 1; i++)
    {
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

    // Óñòàíàâëèâàåì øèðèíó âûâîäà äëÿ êàæäîãî ýëåìåíòà
    int fieldWidth = 10; // Øèðèíà ïîëÿ âûâîäà, ìîæíî íàñòðîèòü ïî âàøåìó óñìîòðåíèþ

    // Çàïèñûâàåì ìàòðèöó â ôàéë ñ âûðàâíèâàíèåì ñòîëáöîâ
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            outputFile << std::setw(fieldWidth) << std::fixed << std::setprecision(4) << matrix[i * N + j];
        }
        outputFile << std::endl;
    }

    outputFile.close();
}


int main(int argc, char const* argv[])
{
    // ïàðñèì àðãóìåíòû
    opt::options_description desc("îïöèè");
    desc.add_options()
        ("accuracy", opt::value<double>()->default_value(1e-6), "òî÷íîñòü")
        ("cellsCount", opt::value<int>()->default_value(256), "ðàçìåð ìàòðèöû")
        ("iterCount", opt::value<int>()->default_value(1000000), "êîëè÷åñòâî îïåðàöèé")
        ("help", "ïîìîùü")
        ;

    opt::variables_map vm;

    opt::store(opt::parse_command_line(argc, argv, desc), vm);

    opt::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << "\n";
        return 1;
    }


    // è ýòî âñ¸ áûëî òîëüêî ðàäè òîãî ÷òîáû ñïàðñèòü àðãóìåíòû.......

    int N = vm["cellsCount"].as<int>();
    double accuracy = vm["accuracy"].as<double>();
    int countIter = vm["iterCount"].as<int>();

    double error = 1.0;
    int iter = 0;

    std::unique_ptr<double[]> A(new double[N * N]);
    std::unique_ptr<double[]> Anew(new double[N * N]);


    initMatrix(std::ref(A), N);
    initMatrix(std::ref(Anew), N);

    auto start = std::chrono::high_resolution_clock::now();
    double* curmatrix = A.get();
    double* prevmatrix = Anew.get();

#pragma acc data copyin(error,prevmatrix[0:N*N],curmatrix[0:N*N])
    {
        while (iter < countIter && iter<10000000 && error > accuracy) {
            // ïðîâåðåíîå ýêñïåðåìåíòàëüíûì ïóò¸ì îïòèìàëüíîå êîëè÷åñòâî áàíä è ðàçìåð âåêòîðà äëÿ ðàñ÷¸òà ìàòðèöû 1024^2
#pragma acc parallel loop independent collapse(2) present(curmatrix,prevmatrix)
            for (size_t i = 1; i < N - 1; i++)
            {

                for (size_t j = 1; j < N - 1; j++)
                {
                    curmatrix[i * N + j] = 0.25 * (prevmatrix[i * N + j + 1] + prevmatrix[i * N + j - 1] + prevmatrix[(i - 1) * N + j] + prevmatrix[(i + 1) * N + j]);
                }
            }




            if ((iter + 1) % 10000 == 0) {
                error = 0.0;
#pragma acc update device(error)
#pragma acc parallel loop independent collapse(2) present(curmatrix,prevmatrix)
                for (size_t i = 1; i < N - 1; i++)
                {
                    for (size_t j = 1; j < N - 1; j++)
                    {
                        error = fmax(error, fabs(curmatrix[i * N + j] - prevmatrix[i * N + j]));
                    }

                }

#pragma acc update self(error)
                std::cout << "iteration: " << iter + 1 << ' ' << "error: " << error << std::endl;
            }


            double* temp = prevmatrix;
            prevmatrix = curmatrix;
            curmatrix = temp;





            iter++;


        }
#pragma acc update self(curmatrix[0:N*N])
    }


    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;
    auto time_s = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();


    std::cout << "time: " << time_s << " error: " << error << " iterarion: " << iter << std::endl;

    if (N <= 13) {

        for (size_t i = 0; i < N; i++)
        {
            for (size_t j = 0; j < N; j++)
            {
                /* code */
                std::cout << A[i * N + j] << ' ';

            }
            std::cout << std::endl;
        }
    }
    saveMatrixToFile(std::ref(curmatrix), N, "matrix.txt");
    A = nullptr;
    Anew = nullptr;




    return 0;
}
