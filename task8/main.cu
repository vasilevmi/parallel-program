#include <iostream>
#include <boost/program_options.hpp>
#include <cmath>
#include <memory>
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <chrono>
#include <cub/cub.cuh>
#include <cuda_runtime.h>

namespace opt = boost::program_options;

template <class ctype>
class Data {
private:
    int len;
    ctype* d_arr;// указатель на массив на GPU

public:
    std::vector<ctype> arr;//сам массив на cpu(хосте)

    Data(int length) : len(length), arr(len), d_arr(nullptr) { //объявление конструктора класса и иниц членов класса . d_arr(nullptr) инициализирует указатель d_arr значением nullptr
        cudaError_t err = cudaMalloc((void**)&d_arr, len * sizeof(ctype));//err хранит код ошибки
        if (err != cudaSuccess) {
            std::cerr << "CUDA memory allocation failed: " << cudaGetErrorString(err) << std::endl;//возвращает строку с описанием ошибки
            exit(EXIT_FAILURE);
        }
    }

    ~Data() {
        if (d_arr) {
            cudaFree(d_arr);//освобождает память, выделенную для массива на устройстве
        }
    }
    void copyToDevice() {       //данные будут копироваться с хоста на устройство
        cudaError_t err = cudaMemcpy(d_arr, arr.data(), len * sizeof(ctype), cudaMemcpyHostToDevice);//указатель на начало памяти на устройстве (GPU); arr.data() возвращает указатель на начало массива данных вектора.
        if (err != cudaSuccess) {
            std::cerr << "CUDA memory copy to device failed: " << cudaGetErrorString(err) << std::endl;
            exit(EXIT_FAILURE);
        }
    }
    void copyToHost() {//данные будут копироваться с устройства на хост 
        cudaError_t err = cudaMemcpy(arr.data(), d_arr, len * sizeof(ctype), cudaMemcpyDeviceToHost);// len(длинa массива) * sizeof(ctype)-размер каждого элемента в байтах
        if (err != cudaSuccess) {
            std::cerr << "CUDA memory copy to host failed: " << cudaGetErrorString(err) << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    ctype* getDevicePointer() { //отдает ссылку на массив на устройстве(гпу) 
        return d_arr;
    }
};

void write_matrix(const double* curmatrix, int N, const std::string& filename) {
    std::ofstream outputFile(filename);
    int fieldWidth = 10;

    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            outputFile << std::setw(fieldWidth) << std::fixed << std::setprecision(4) << curmatrix[i * N + j];
        }
        outputFile << std::endl;
    }

    outputFile.close();
}

double linearInterpolation(double x, double x1, double y1, double x2, double y2) {
    return y1 + ((x - x1) * (y2 - y1) / (x2 - x1));
}



void init(Data<double>& curmatrix, int size) {
    for (int i = 0; i < size; ++i) {
        for (int j = 0; j < size; ++j) {
            curmatrix.arr[i * size + j] = 0;
        }
    }
    curmatrix.arr[0] = 10.0;
    curmatrix.arr[size - 1] = 20.0;
    curmatrix.arr[(size - 1) * size + (size - 1)] = 30.0;
    curmatrix.arr[(size - 1) * size] = 20.0;
    for (int i = 1; i < size - 1; ++i) {
        curmatrix.arr[i * size + 0] = linearInterpolation(i, 0.0, curmatrix.arr[0], size - 1, curmatrix.arr[(size - 1) * size]);
    }
    for (int i = 1; i < size - 1; ++i) {
        curmatrix.arr[0 * size + i] = linearInterpolation(i, 0.0, curmatrix.arr[0], size - 1, curmatrix.arr[size - 1]);
    }
    for (int i = 1; i < size - 1; ++i) {
        curmatrix.arr[(size - 1) * size + i] = linearInterpolation(i, 0.0, curmatrix.arr[(size - 1) * size], size - 1, curmatrix.arr[(size - 1) * size + (size - 1)]);
    }
    for (int i = 1; i < size - 1; ++i) {
        curmatrix.arr[i * size + (size - 1)] = linearInterpolation(i, 0.0, curmatrix.arr[size - 1], size - 1, curmatrix.arr[(size - 1) * size + (size - 1)]);
    }
}

__global__ void iterate(double* curmatrix, double* prevmatrix, int size) { // определяем ядро CUDA с именем iterate. ядро CUDA — это функция, которая выполняется параллельно на устройстве 
    int j = blockIdx.x * blockDim.x + threadIdx.x;//представляет горизонтальную координату
    int i = blockIdx.y * blockDim.y + threadIdx.y;//i — вертикальная координата текущего элемента в массиве.

    if (j == 0 || i == 0 || i >= size - 1 || j >= size - 1) return;//проверяет, находится ли текущий элемент на границе матрицы.Если да то текущий поток не будет обрабатывать данный элемент матрицы, а завершится без выполнения оставшейся части кода

    curmatrix[i * size + j] = 0.25 * (prevmatrix[i * size + j + 1] + prevmatrix[i * size + j - 1] +
        prevmatrix[(i - 1) * size + j] + prevmatrix[(i + 1) * size + j]);
}//Таким образом, это ядро CUDA выполняет одну итерацию вычислений на каждом элементе матрицы

template <unsigned int blockSize>
__global__ void compute_error(double* curmatrix, double* prevmatrix, double* max_error, int size) { //объявление ядра CUDA с именем compute_error
    int j = blockIdx.x * blockDim.x + threadIdx.x;
    int i = blockIdx.y * blockDim.y + threadIdx.y;

    if (j >= size || i >= size) return;//Если координаты элемента находятся за пределами размеров матрицы, то прекращается выполнение текущего потока.

    __shared__ typename cub::BlockReduce<double, blockSize>::TempStorage temp_storage;//Общая память доступна только для потоков, выполняющихся внутри одного блока. TempStorage temp_storage для хранения временных данных, необходимых для выполнения операции редукции на уровне блока
    double local_max = 0.0;// максимальное значение ошибки для каждого потока в блоке.


    if (j > 0 && i > 0 && j < size - 1 && i < size - 1) { //вычисляет ошибку для каждого элемента внутри матрицы, за исключением граничных элементов
        int index = i * size + j;
        double error = fabs(curmatrix[index] - prevmatrix[index]);
        local_max = error ;   }

    //  block_max вычисляет максимальное значение ошибки из всех значений в блоке.
    double block_max = cub::BlockReduce<double, blockSize>(temp_storage).Reduce(local_max, cub::Max());// BlockReduce  используется для выполнения операций редукции на уровне блока ,Max(), которая находит максимальное значение среди всех значений

    if (threadIdx.x == 0 && threadIdx.y == 0) { // является ли текущий поток первым в блоке. !Каждый блок должен записать максимальное значение ошибки только один раз, так как это значение представляет собой максимальную ошибку для всего блока, а не для каждого потока в блоке. 
        int block_index = blockIdx.y * gridDim.x + blockIdx.x;
        max_error[block_index] = block_max;
    }
}

struct CudaFreeDeleter { //для освобождения памяти, выделенной с помощью функции cudaMalloc
    void operator()(void* ptr) const {
        cudaFree(ptr);
    }
};

struct StreamDeleter {//для уничтожения потока, а затем удаляет указатель.


    void operator()(cudaStream_t* stream) {
        cudaStreamDestroy(*stream);
        delete stream;
    }
};

struct GraphDeleter { //для уничтожения графа, а затем удаляет указатель.
    void operator()(cudaGraph_t* graph) {
        cudaGraphDestroy(*graph);
        delete graph;
    }
};

struct GraphExecDeleter { //lля уничтожения исполняемого графа, а затем удаляет указатель.
    void operator()(cudaGraphExec_t* graphExec) {
        cudaGraphExecDestroy(*graphExec);
        delete graphExec;
    }
};

int main(int argc, char const* argv[]) {
    opt::options_description desc("Arguments");
    desc.add_options()
        ("accuracy", opt::value<double>()->default_value(1e-6), "accuracy")
        ("matr_size", opt::value<int>()->default_value(256), "matrix_size")
        ("num_iter", opt::value<int>()->default_value(1000000), "num_iterations")
        ("help", "help");

    opt::variables_map vm;
    opt::store(opt::parse_command_line(argc, argv, desc), vm);
    opt::notify(vm);// значения опций были успешно разобраны и сохранены в vm
    if (vm.count("help")) {
        std::cout << desc << "\n";
        return 1;
    }

    auto start = std::chrono::high_resolution_clock::now();
    int size = vm["matr_size"].as<int>();
    double accuracy = vm["accuracy"].as<double>();
    int countIter = vm["num_iter"].as<int>();

    Data<double> curmatrix(size * size);
    Data<double> prevmatrix(size * size);

    init(curmatrix, size);// иницилизация значений матрицы 
    init(prevmatrix, size);

    double error;
    error = accuracy + 1;
    int iter = 0;

    dim3 blockDim(32, 32);
    dim3 gridDim((size + blockDim.x - 1) / blockDim.x, (size + blockDim.y - 1) / blockDim.y);// определяет размеры сетки. (size + blockDim.x - 1) / blockDim.x: Это вычисляет количество блоков по оси X.(size + blockDim.y - 1) / blockDim.y) по у

    Data<double> d_max_error(gridDim.x * gridDim.y);// массив ошибок размерности сетки gridDim.x * gridDim.y
    Data<double> d_final_max_error(1);
    void* d_temp_storage = nullptr;// указатель, который будет использоваться для выделения памяти на устройстве для временного хранения данных в ходе выполнения операции редукции.
    size_t temp_storage_bytes = 0;//будет содержать количество байтов, необходимых для выделения памяти для временного хранения данных в ходе операции редукции

    cub::DeviceReduce::Max(d_temp_storage, temp_storage_bytes, d_max_error.getDevicePointer(), d_final_max_error.getDevicePointer(), gridDim.x * gridDim.y);//для выполнения операции редукции на GPU, чтобы найти максимальное значение из данных, хранящихся в d_max_error
    std::unique_ptr<void, CudaFreeDeleter> d_temp_storage_unique;//для автоматического освобождения памяти на устройстве редукции
    cudaError_t err = cudaMalloc(&d_temp_storage, temp_storage_bytes);// выделяет память на устройстве для временного хранения данных в ходе операции редукции
    if (err != cudaSuccess) {
        std::cerr << "CUDA memory allocation failed: " << cudaGetErrorString(err) << std::endl;
        exit(EXIT_FAILURE);
    }
    d_temp_storage_unique.reset(d_temp_storage);//гарантирует, что память будет автоматически освобождена когда объект d_temp_storage_unique будет уничтожен.

    d_max_error.copyToDevice();
    curmatrix.copyToDevice();
    prevmatrix.copyToDevice();


    //получают указатели на данные, хранящиеся на устройстве.Полезно для передачи этих указателей в качестве аргументов для вызова функций CUDA.
    double* curmatrix_adr = curmatrix.getDevicePointer();
    double* prevmatrix_adr = prevmatrix.getDevicePointer();
    double* d_max_error_adr = d_max_error.getDevicePointer();
    double* d_final_max_error_adr = d_final_max_error.getDevicePointer();

    //Cоздают уникальные указатели для объектов типа cudaStream_t, cudaGraph_t и cudaGraphExec_t
    std::unique_ptr<cudaStream_t, StreamDeleter> stream(new cudaStream_t);
    std::unique_ptr<cudaGraph_t, GraphDeleter> graph(new cudaGraph_t);//cudaGraph_t представляет собой тип данных из библиотеки CUDA, используемый для представления графа вычислений на GPU
    std::unique_ptr<cudaGraphExec_t, GraphExecDeleter> graphExec(new cudaGraphExec_t);// для выполнения графа вычислений на GPU

    cudaStreamCreate(stream.get());//создает новый поток stream.get() для получения обычного указателя на объект типа cudaStream_t
    bool graphCreated = false;

    cudaMemset(d_max_error_adr, 0, sizeof(double));// все байты в этой области памяти устанавливаются в 0

    double final_error;


    while (iter < countIter && error > accuracy) { //цикл, который выполняет итерации алгоритма до достижения заданной точности или максимального количества итераций
        if (!graphCreated) {// если граф не создан
            cudaStreamBeginCapture(*stream, cudaStreamCaptureModeGlobal);//захватываются все операции в потоке, включая ядра CUDA, копирование памяти и другие действия.

            for (int i = 0; i < 999; i++) {
                iterate << <gridDim, blockDim, 0, *stream >> > (curmatrix_adr, prevmatrix_adr, size);//Выполняется серия итераций редукции, представленного в ядре iterate, используя поток stream
                std::swap(prevmatrix_adr, curmatrix_adr);
            }

            iterate << <gridDim, blockDim, 0, *stream >> > (curmatrix_adr, prevmatrix_adr, size);//Это запуск ядра iterate. iterate обрабатывает входные данные (текущее и предыдущее состояния матрицы),
            compute_error<32> << <gridDim, blockDim, 0, *stream >> > (curmatrix_adr, prevmatrix_adr, d_max_error_adr, size);// запуск ядра compute_error. compute_error вычисляет ошибку в результате этих вычислений и записывает ее в d_max_error_adr

            cudaStreamEndCapture(*stream, graph.get());//завершает захват операций
            cudaGraphInstantiate(graphExec.get(), *graph, nullptr, nullptr, 0);//cоздает  граф вычислений на основе захваченных операций. Указатель на объект графа CUDA, который был захвачен и завершен.

            graphCreated = true;//граф вычислений был создан
        }
        else {
            cudaGraphLaunch(*graphExec, *stream);//Граф вычислений запускается для выполнения в потоке stream.
          
            cub::DeviceReduce::Max(d_temp_storage, temp_storage_bytes, d_max_error_adr, d_final_max_error_adr, gridDim.x * gridDim.y, *stream);//используется операция редукции для вычисления максимальной ошибки среди всех элементов массива d_max_error_adr на GPU.
            cudaMemcpy(&final_error, d_final_max_error_adr, sizeof(double), cudaMemcpyDeviceToHost);//Максимальная ошибка копируется с устройства на хост и сохраняется в переменной error
            error = final_error;

            std::cout << "Iteration: " << iter + 1000 << ", Error: " << error << std::endl;

            iter += 1000;
        }
    }

    curmatrix.copyToHost();
    auto end = std::chrono::high_resolution_clock::now();
    auto time_s = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    std::cout << "time: " << time_s << " error: " << error << " iteration: " << iter << std::endl;

    if (size <= 13) {
        for (size_t i = 0; i < size; i++) {
            for (size_t j = 0; j < size; j++) {
                std::cout << curmatrix.arr[i * size + j] << ' ';
            }
            std::cout << std::endl;
        }
    }

    write_matrix(curmatrix.arr.data(), size, "matrix2.txt");

    return 0;
}
