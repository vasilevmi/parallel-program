<<<<<<< HEAD
#include <iostream>
#include <thread>
#include <list>
#include <mutex>
#include <queue>
#include <future>
#include <functional>
#include <condition_variable>
#include <random>
#include <cmath> 
#include <unordered_map>

template <typename T>

class TaskServer
{
public:
    void start_server()// ������a�� ������� 
    {
        stop_flag_ = false;//�� ��������, ��� ������ ����� ������� � ���������� ������
        server_thread = std::thread(&TaskServer::server_thread, this);//������ ����� ����� ����������, ������� ����� ��������� ����� server_thread,this ��� ��������� � �������� 
    }

    void stop_server()//������������ �������
    {
        {
            std::unique_lock<std::mutex> lock(mutex_);//��� �������� ������� lock ������ ������, ���������� ������������� ��� �� �������, ����� �������, ���� lock �� ����� �������������.
            stop_flag_ = true;//���� ������ ��������� ������ ������� � ���, ��� ������ ������ ���� ����������.
            cv.notify_one(); //����� �� ���������� �����, ��� ������ ������ ���� ����������
        }
        server_thread.join();//���� ������ join(), ������� ����� ����� ������������ � ����� �������, ���� ����� server_thread_ �� �������� ���� ����������
    }
    size_t add_task_to_queue(std::function<T()> task)
    {
        std::unique_lock<std::mutex> lock(mutex_);// ����������� ������� ��� ����������� ������� � ����� ������
        size_t task_id = ++id;// ���������� ���������� ������������� ��� ������
        auto future = std::async(std::launch::deferred, task); // ������� ����������� ������, � task ����� ������ ������ ������� ����� ����������� ��������� 
        tasks.push({ task_id, std::move(future) });// ��������� ���� �������������� ������ � �� future � ������� �����
        cv.notify_one();  // ���������� ���� �� �������, ��� ������ ������ ���� ����������
        return task_id;
    }
    T request_result(size_t id)//������ ���������� ���������� ������ �� � ��������������
    {
        //std::lock_guard<std::mutex> lock(mutex_);//lock_guard ������������� ����������� ������� 
        //cv.wait(lock, [this, id]() {return results.find(id) != results.end(); }); // ���������, ���� �� ��������� � ��������� id � ����������,����� ������� �����������, ����� ��������� ����������.
        //T result = results[id];//�������� ��������� �� ���������� results_ �� ���������� id
        //results.erase(id);// ������� ������� � ��������� id �� ����������
        //return result;
        std::unique_lock<std::mutex> lock(mutex_);
        cv.wait(lock, [this, id]() {return results.find(id) != results.end(); }); // ��� ������ �� ������ �������
        T result = results[id];
        results.erase(id);
        return result;
   
    }
private:
    std::mutex mutex_;
    std::condition_variable cv;
    std::thread server_thread;
    bool stop_flag_ = false;
    size_t id = 1;
    std::queue<std::pair<size_t, std::future<T>>> tasks;
    std::unordered_map<size_t, T> results;
    void serverThread()//�������� �� ��������� ����� �� �������
    {
        while (true) {
            std::unique_lock<std::mutex> lock(mutex_);
            cv.wait(lock, [this] { return !tasks.empty() || stop_flag_; });//cv ����� �������������� ��� ����������� ��������� ������ � ������� ����� � �������

            if (tasks.empty() && stop_flag_) { //���� ��������� ������ � ���������� ����
                break;
            }

            if (!tasks.empty()) {//���� ������� �� ����� 
                auto task = std::move(tasks.front());//����������� ������ ������ �� ������� 
                tasks.pop();
                results[task.first] = task.second.get();
                cv.notify_one();//����������� ���� ������ ������ ����� ���������� ���� ������.
            }
        }
    }
};


template <typename T>
class TaskClient {
public:
    void run_client(TaskServer<T>& server, std::function<T()> task_generator)  // ������ ������� ����� � �������� �������-�����������
    {
        int id = server.add_task_to_queue(task_generator);//���������� ������ ��������������� �������� task_generator
        task_ids_.push_back(id);
    }

    // ��������� ����������� ���������� ����� �������
    std::list<T> client_to_result(TaskServer<T>& server) //�������� ���������� ���������� �����
    {
        std::list<T> results;
        for (int id : task_ids_) {
            T result = server.request_result(id);
            results.push_back(result);
        }
        return results;
    }
private:
    std::vector<int> task_ids_;
};

//������ 1 ���������� ��� ���������� ����� 
template<typename T>
T random_sin() {

    static std::default_random_engine generator;
    static std::uniform_real_distribution<T> distribution(-3.14159, 3.14159);
    T x = distribution(generator);
    return std::sin(x);
}

//������ 2 ���������� �����  ���������� ����� 
template<typename T>
T random_sqrt() {
    static std::default_random_engine generator;
    static std::uniform_real_distribution<T> distribution(1.0, 10.0);
    T value = distribution(generator);
    return std::sqrt(value);
}

//������ 3 ���������� ��������  ���������� ����� 
template<typename T>
T random_pow() {
    static std::default_random_engine generator;
    static std::uniform_real_distribution<T> distribution(1.0, 10.0);
    T value = distribution(generator);
    return std::pow(value, 2.0);
}


int main() {
    TaskServer<double> server;
    server.start_server();

    TaskClient<double> client1;
    TaskClient<double> client2;
    TaskClient<double> client3;

    for (int i = 0; i < 5; ++i) {
        client1.run_client(server, random_sin<double>);
        client2.run_client(server, random_sqrt<double>);
        client3.run_client(server, random_pow<double>);

    }

    std::list<double> t1 = client1.client_to_result(server);
    std::list<double> t2 = client2.client_to_result(server);
    std::list<double> t3 = client3.client_to_result(server);

    std::cout << "t1: ";
    for (double n : t1)
        std::cout << n << ", ";
    std::cout << "\n";

    std::cout << "t2: ";
    for (double n : t2)
        std::cout << n << ", ";
    std::cout << "\n";

    std::cout << "t3: ";
    for (double n : t3)
        std::cout << n << ", ";
    std::cout << "\n";

    server.stop_server();

    return 0;
}
=======
#include <iostream>
#include <fstream>
#include <thread>
#include <list>
#include <mutex>
#include <queue>
#include <future>
#include <functional>
#include <condition_variable>
#include <random>
#include <cmath>
#include <unordered_map>
 

template <typename T>
class CustomTaskServer
{
public:
    void startServer() {
        stopFlag = false;// Флаг остановки сервера
        serverThread = std::thread(&CustomTaskServer::serverThreadFunction, this); // Запуск потока, который будет обрабатывать задачи
    }

    void stopServer() {
        {
            std::unique_lock<std::mutex> lock(mutex_);//переменной, который представляет уникальную блокировку мьютекса( гарантирует, что только один поток может одновременно обращаться к защищенному ресурсу.)
            stopFlag = true;
            cv.notify_one();// Уведомляем поток сервера, чтобы он проснулся
        }
        serverThread.join();// Ожидаем завершения работы потока сервера
    }

    size_t addTaskQueue(std::function<T()> task) {
        std::unique_lock<std::mutex> lock(mutex_);
        tasks.push({ id++, std::async(std::launch::deferred, task) });// Добавляем задачу в очередь
        cv.notify_one();// Уведомляем поток сервера о наличии новой задачи
        return tasks.back().first;// Возвращаем идентификатор задачи
    }

    T requestResult(size_t id) {// Метод для запроса результата выполнения задачи по ее идентификатору
        std::unique_lock<std::mutex> lock(mutex_);
        cv.wait(lock, [this, id]() { return results.find(id) != results.end(); });// Ждем, пока результат выполнения задачи будет доступен
        T result = results[id]; // Получаем результат выполнения задачи
        results.erase(id);// Удаляем результат из контейнера
        return result;
    }

private:
    std::mutex mutex_;// Мьютекс для синхронизации доступа к данным
    std::condition_variable cv;// Условная переменная для ожидания событий
    std::thread serverThread;
    bool stopFlag = false;// Флаг остановки сервера
    size_t id = 1;// Идентификатор задачи
    std::queue<std::pair<size_t, std::future<T>>> tasks;// Очередь
    std::unordered_map<size_t, T> results; // Контейнер для хранения результатов выполнения задач

    void serverThreadFunction() {  // Функция потока для обработки задач
        while (true) {
            std::unique_lock<std::mutex> lock(mutex_);
            cv.wait(lock, [this] { return !tasks.empty() || stopFlag; }); // Ожидаем появления новой задачи или сигнала об остановке сервера

            if (tasks.empty() && stopFlag) {  // Если очередь пуста и сервер должен быть остановлен, выходим из цикла
                break;
            }

            if (!tasks.empty()) { // Если очередь не пуста, извлекаем задачу и выполняем ее
                auto task = std::move(tasks.front());// Извлекаем задачу из очереди
                tasks.pop();// Удаляем задачу из очереди
                results[task.first] = task.second.get();// Получаем результат выполнения задачи и сохраняем его
                cv.notify_one();// Уведомляем о наличии результата
            }
        }
    }
};

template <typename T>
class CustomTaskClient {
public:
    void runClient(CustomTaskServer<T>& server, std::function<std::pair<T, T>()> taskGenerator) { // Метод для запуска клиента
        auto task = taskGenerator();// Генерируем задачу
        int id = server.addTaskQueue([task]() { return task.second; });// Добавляем задачу в очередь на сервере
        taskIds.push_back({ id, task.first });// Сохраняем идентификатор задачи
    }

    std::list<std::pair<T, T>> clientToResult(CustomTaskServer<T>& server) // Метод для получения результатов выполнения задач клиентом
    {
        std::list<std::pair<T, T>> results;// Контейнер для результатов
        for (const auto& taskId : taskIds) {// Для каждого идентификатора задачи запрашиваем результат выполнения
            T result = server.requestResult(taskId.first);// Запрашиваем результат
            results.push_back(std::make_pair(taskId.first, result));// Сохраняем результат
        }
        return results;
    }
private:
    std::vector<std::pair<int, T>> taskIds;
};

template<typename T>
std::pair<T, T> randomSin()
{
    static std::default_random_engine generator;
    static std::uniform_real_distribution<T> distribution(-3.14159, 3.14159);
    T value = distribution(generator);
    return { value, std::sin(value) };
}

// Задача 2: Вычисление корня из случайного числа 
template<typename T>
std::pair<T, T> randomSqrt() {
    static std::default_random_engine generator;
    static std::uniform_real_distribution<T> distribution(1.0, 10.0);
    T value = distribution(generator);
    return { value, std::sqrt(value) };
}

// Задача 3: Вычисление квадрата случайного числа 
template<typename T>
std::pair<T, T> randomPow() {
    static std::default_random_engine generator;
    static std::uniform_real_distribution<T> distribution(1.0, 10.0);
    T value = distribution(generator);
    return { value, std::pow(value, 2.0) };
}

int main() {
    CustomTaskServer<double> server;
    server.startServer();

    CustomTaskClient<double> client1;
    CustomTaskClient<double> client2;
    CustomTaskClient<double> client3;

    for (int i = 0; i < 10000; ++i) {
        client1.runClient(server, randomSin<double>);
        client2.runClient(server, randomSqrt<double>);
        client3.runClient(server, randomPow<double>);
    }

    std::list<std::pair<double, double>> t1 = client1.clientToResult(server);
    std::list<std::pair<double, double>> t2 = client2.clientToResult(server);
    std::list<std::pair<double, double>> t3 = client3.clientToResult(server);

    server.stopServer();

    // Сохранение результатов в файлы
    std::ofstream file1("t1.txt");
    for (const auto& p : t1)
        file1 << "sin " << p.first << " " << p.second << std::endl;
    file1.close();

    std::ofstream file2("t2.txt");
    for (const auto& p : t2)
        file2 << "sqrt " << p.first << " " << p.second << std::endl;
    file2.close();

    std::ofstream file3("t3.txt");
    for (const auto& p : t3)
        file3 << "pow " << p.first << " " << p.second << std::endl;
    file3.close();

    return 0;
}
>>>>>>> 26876ca38acc82ac3598bc4cf1deb0c9e8a48e74
