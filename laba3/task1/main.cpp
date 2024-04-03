#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <memory>

using namespace std;


double cpuSecond()
{
    using namespace std::chrono;
    system_clock::time_point now = system_clock::now();//получаем текущее время 
    system_clock::duration tp = now.time_since_epoch();//получаем длительность (duration) от начала эпохи
    return duration_cast<duration<double>>(tp).count();//преобразуем полученную длительность в секунды 
}

void mult_matrix_vector(double* a, double* b, double* c, int m, int n,int count_threads)//указатели на матрицу a, вектор b, и вектор c, а также размерность матрицы m x n.
{
    for (int i = 0; i < m; i++)
    {
        c[i] = 0.0;
        for (int j = 0; j < n; j++)
        {
            c[i] += a[i * n + j] * b[j];
        }
    }
}

void mult_matrix_vector_parallel(double* a, double* b, double* c, int m, int n, int threadid, int items_per_thread,int count_threads)//идентификатор потока threadid и количество элементов, обрабатываемых одним потоком items_per_thread.
{
    int lb = threadid * items_per_thread;//(lower bound) - нижняя граница индексов строк, обрабатываемых текущим потоком.
    int ub = (threadid == count_threads- 1) ? (m - 1) : (lb + items_per_thread - 1);// (upper bound) - верхняя граница индексов строк, обрабатываемых текущим потоком

    for (int i = lb; i <= ub; i++)
    {
        c[i] = 0.0;
        for (int j = 0; j < n; j++)
            c[i] += a[i * n + j] * b[j];
    }
}

void run_serial(std::unique_ptr<double[]>& a, std::unique_ptr<double[]>& b, std::unique_ptr<double[]>& c, int n, int m,int count_threads)
{

    for (int i = 0; i < m; i++)
    {
        for (int j = 0; j < n; j++)
            a[i * n + j] = i + j;//заполнение элементов матрицы a суммой индексов строки i и столбца j.
    }

    for (int j = 0; j < n; j++)
        b[j] = j;//Заполнение вектора b значениями от 0 до n-1

    double t = cpuSecond();
    mult_matrix_vector(a.get(), b.get(), c.get(), n, m, count_threads);// вычисления произведения матрицы на вектор
    t = cpuSecond() - t;

    std::cout << "Elapsed time (serial): " << t << " sec." << std::endl;
}
void run_parallel(std::unique_ptr<double[]>& a, std::unique_ptr<double[]>& b, std::unique_ptr<double[]>& c, int n, int m,int count_threads)
{
  

    double start = cpuSecond(); // время до запуска потоков

    std::vector<std::thread> threads;//Создание вектора потоков для хранения потоков.
    int items_per_thread = m / count_threads;

    for (int i = 0; i < count_threads; i++)//Цикл создания count_threads потоков
    {
        threads.push_back(std::thread(mult_matrix_vector_parallel, a.get(), b.get(), c.get(), m, n, i, items_per_thread, count_threads));//оздание потока, который вызывает функцию mult_matrix_vector_parallel i индекс потока
    }

    for (auto& thread : threads)//Цикл ожидания завершения всех потоков.
    {
        thread.join();// В данном случае, для каждого потока в векторе threads будет вызываться метод join(), который ожидает завершения потока.
    }

    double t_end = cpuSecond(); // Замеряем время после завершения всех потоков

    std::cout << "Elapsed time (parallel): " << t_end - start << " sec." << std::endl;
}



int main(int argc, char* argv[])
{
    int m = 20000;
    int n = 20000;
    int count_threads= 40;
    std::unique_ptr<double[]> a(new double[m * n]);
    std::unique_ptr<double[]> b(new double[n]);
    std::unique_ptr<double[]> c(new double[m]);
    if (argc > 1)
        m = atoi(argv[1]);
    if (argc > 2)
        n = atoi(argv[2]);
    if (argc > 3)
       int  counter = atoi(argv[3]);

    run_serial(a, b, c,m, n, count_threads);
    run_parallel(a, b, c, m, n, count_threads);

    return 0;
}
