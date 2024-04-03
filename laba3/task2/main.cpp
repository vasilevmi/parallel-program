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
