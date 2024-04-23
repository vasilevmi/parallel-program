#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <memory>

using namespace std;


double cpuSecond()
{
    using namespace std::chrono;
    system_clock::time_point now = system_clock::now();//�������� ������� ����� 
    system_clock::duration tp = now.time_since_epoch();//�������� ������������ (duration) �� ������ �����
    return duration_cast<duration<double>>(tp).count();//����������� ���������� ������������ � ������� 
}

void mult_matrix_vector(double* a, double* b, double* c, int m, int n,int count_threads)//��������� �� ������� a, ������ b, � ������ c, � ����� ����������� ������� m x n.
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

void mult_matrix_vector_parallel(double* a, double* b, double* c, int m, int n, int threadid, int items_per_thread,int count_threads)//������������� ������ threadid � ���������� ���������, �������������� ����� ������� items_per_thread.
{
    int lb = threadid * items_per_thread;//(lower bound) - ������ ������� �������� �����, �������������� ������� �������.
    int ub = (threadid == count_threads- 1) ? (m - 1) : (lb + items_per_thread - 1);// (upper bound) - ������� ������� �������� �����, �������������� ������� �������

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
            a[i * n + j] = i + j;//���������� ��������� ������� a ������ �������� ������ i � ������� j.
    }

    for (int j = 0; j < n; j++)
        b[j] = j;//���������� ������� b ���������� �� 0 �� n-1

    double t = cpuSecond();
    mult_matrix_vector(a.get(), b.get(), c.get(), n, m, count_threads);// ���������� ������������ ������� �� ������
    t = cpuSecond() - t;

    std::cout << "Elapsed time (serial): " << t << " sec." << std::endl;
}
void run_parallel(std::unique_ptr<double[]>& a, std::unique_ptr<double[]>& b, std::unique_ptr<double[]>& c, int n, int m,int count_threads)
{
  

    double start = cpuSecond(); // ����� �� ������� �������

    std::vector<std::thread> threads;//�������� ������� ������� ��� �������� �������.
    int items_per_thread = m / count_threads;

    for (int i = 0; i < count_threads; i++)//���� �������� count_threads �������
    {
        threads.push_back(std::thread(mult_matrix_vector_parallel, a.get(), b.get(), c.get(), m, n, i, items_per_thread, count_threads));//������� ������, ������� �������� ������� mult_matrix_vector_parallel i ������ ������
    }

    for (auto& thread : threads)//���� �������� ���������� ���� �������.
    {
        thread.join();// � ������ ������, ��� ������� ������ � ������� threads ����� ���������� ����� join(), ������� ������� ���������� ������.
    }

    double t_end = cpuSecond(); // �������� ����� ����� ���������� ���� �������

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
