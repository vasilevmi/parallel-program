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
    void start_server()// Запускaет сервера 
    {
        stop_flag_ = false;//то означает, что сервер задач активен и продолжает работу
        server_thread = std::thread(&TaskServer::server_thread, this);//задает новый поток исполнения, который будет выполнять метод server_thread,this для обращения к текущему 
    }

    void stop_server()//Останвливает сервера
    {
        {
            std::unique_lock<std::mutex> lock(mutex_);//При создании объекта lock другие потоки, пытающиеся заблокировать тот же мьютекс, будут ожидать, пока lock не будет разблокирован.
            stop_flag_ = true;//дает сигнал основному потоку сервера о том, что сервер должен быть остановлен.
            cv.notify_one(); //здесь мы уведомляем поток, что сервер должен быть остановлен
        }
        server_thread.join();//осле вызова join(), текущий поток будет заблокирован и будет ожидать, пока поток server_thread_ не завершит свое выполнение
    }
    size_t add_task_to_queue(std::function<T()> task)
    {
        std::unique_lock<std::mutex> lock(mutex_);// Захватываем мьютекс для безопасного доступа к общим данным
        size_t task_id = ++id;// Генерируем уникальный идентификатор для задачи
        auto future = std::async(std::launch::deferred, task); // создаем асинхронную задачу, в task будет лечать задача которую хотим ассинхронно выполнить 
        tasks.push({ task_id, std::move(future) });// Добавляем пару идентификатора задачи и ее future в очередь задач
        cv.notify_one();  // Уведомляем один из потоков, что сервер должен быть остановлен
        return task_id;
    }
    T request_result(size_t id)//Запрос результата выполнения задачи по её идентификатору
    {
        //std::lock_guard<std::mutex> lock(mutex_);//lock_guard автоматически захватывает мьютекс 
        //cv.wait(lock, [this, id]() {return results.find(id) != results.end(); }); // проверяет, есть ли результат с указанным id в контейнере,когда условие выполняется, поток продолжит выполнение.
        //T result = results[id];//Получаем результат из контейнера results_ по указанному id
        //results.erase(id);// Удаляем элемент с указанным id из контейнера
        //return result;
        std::unique_lock<std::mutex> lock(mutex_);
        cv.wait(lock, [this, id]() {return results.find(id) != results.end(); }); // ждём ответа от потока сервера
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
    void serverThread()//отвечает за обработку задач из очереди
    {
        while (true) {
            std::unique_lock<std::mutex> lock(mutex_);
            cv.wait(lock, [this] { return !tasks.empty() || stop_flag_; });//cv будет использоваться для уведомления основного потока о наличии задач в очереди

            if (tasks.empty() && stop_flag_) { //если контейнер пустой и установлен флаг
                break;
            }

            if (!tasks.empty()) {//если очередь не пуста 
                auto task = std::move(tasks.front());//извлекается первая задача из очереди 
                tasks.pop();
                results[task.first] = task.second.get();
                cv.notify_one();//оповещается тобы другие потоки могли продолжить свою работу.
            }
        }
    }
};


template <typename T>
class TaskClient {
public:
    void run_client(TaskServer<T>& server, std::function<T()> task_generator)  // Запуск клиента задач с заданной задачей-генератором
    {
        int id = server.add_task_to_queue(task_generator);//добавление задачи сгенерированной функцией task_generator
        task_ids_.push_back(id);
    }

    // Получение результатов выполнения задач клиента
    std::list<T> client_to_result(TaskServer<T>& server) //получает результаты выполнения задач
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

//Задача 1 вычесление син случайного числа 
template<typename T>
T random_sin() {

    static std::default_random_engine generator;
    static std::uniform_real_distribution<T> distribution(-3.14159, 3.14159);
    T x = distribution(generator);
    return std::sin(x);
}

//Задача 2 вычесление корня  случайного числа 
template<typename T>
T random_sqrt() {
    static std::default_random_engine generator;
    static std::uniform_real_distribution<T> distribution(1.0, 10.0);
    T value = distribution(generator);
    return std::sqrt(value);
}

//Задача 3 вычесление квадрата  случайного числа 
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