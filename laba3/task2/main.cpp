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
        stopFlag = false;// Р¤Р»Р°Рі РѕСЃС‚Р°РЅРѕРІРєРё СЃРµСЂРІРµСЂР°
        serverThread = std::thread(&CustomTaskServer::serverThreadFunction, this); // Р—Р°РїСѓСЃРє РїРѕС‚РѕРєР°, РєРѕС‚РѕСЂС‹Р№ Р±СѓРґРµС‚ РѕР±СЂР°Р±Р°С‚С‹РІР°С‚СЊ Р·Р°РґР°С‡Рё
    }

    void stopServer() {
        {
            std::unique_lock<std::mutex> lock(mutex_);//РїРµСЂРµРјРµРЅРЅРѕР№, РєРѕС‚РѕСЂС‹Р№ РїСЂРµРґСЃС‚Р°РІР»СЏРµС‚ СѓРЅРёРєР°Р»СЊРЅСѓСЋ Р±Р»РѕРєРёСЂРѕРІРєСѓ РјСЊСЋС‚РµРєСЃР°( РіР°СЂР°РЅС‚РёСЂСѓРµС‚, С‡С‚Рѕ С‚РѕР»СЊРєРѕ РѕРґРёРЅ РїРѕС‚РѕРє РјРѕР¶РµС‚ РѕРґРЅРѕРІСЂРµРјРµРЅРЅРѕ РѕР±СЂР°С‰Р°С‚СЊСЃСЏ Рє Р·Р°С‰РёС‰РµРЅРЅРѕРјСѓ СЂРµСЃСѓСЂСЃСѓ.)
            stopFlag = true;
            cv.notify_one();// РЈРІРµРґРѕРјР»СЏРµРј РїРѕС‚РѕРє СЃРµСЂРІРµСЂР°, С‡С‚РѕР±С‹ РѕРЅ РїСЂРѕСЃРЅСѓР»СЃСЏ
        }
        serverThread.join();// РћР¶РёРґР°РµРј Р·Р°РІРµСЂС€РµРЅРёСЏ СЂР°Р±РѕС‚С‹ РїРѕС‚РѕРєР° СЃРµСЂРІРµСЂР°
    }

    size_t addTaskQueue(std::function<T()> task) {
        std::unique_lock<std::mutex> lock(mutex_);
        tasks.push({ id++, std::async(std::launch::deferred, task) });// Р”РѕР±Р°РІР»СЏРµРј Р·Р°РґР°С‡Сѓ РІ РѕС‡РµСЂРµРґСЊ
        cv.notify_one();// РЈРІРµРґРѕРјР»СЏРµРј РїРѕС‚РѕРє СЃРµСЂРІРµСЂР° Рѕ РЅР°Р»РёС‡РёРё РЅРѕРІРѕР№ Р·Р°РґР°С‡Рё
        return tasks.back().first;// Р’РѕР·РІСЂР°С‰Р°РµРј РёРґРµРЅС‚РёС„РёРєР°С‚РѕСЂ Р·Р°РґР°С‡Рё
    }

    T requestResult(size_t id) {// РњРµС‚РѕРґ РґР»СЏ Р·Р°РїСЂРѕСЃР° СЂРµР·СѓР»СЊС‚Р°С‚Р° РІС‹РїРѕР»РЅРµРЅРёСЏ Р·Р°РґР°С‡Рё РїРѕ РµРµ РёРґРµРЅС‚РёС„РёРєР°С‚РѕСЂСѓ
        std::unique_lock<std::mutex> lock(mutex_);
        cv.wait(lock, [this, id]() { return results.find(id) != results.end(); });// Р–РґРµРј, РїРѕРєР° СЂРµР·СѓР»СЊС‚Р°С‚ РІС‹РїРѕР»РЅРµРЅРёСЏ Р·Р°РґР°С‡Рё Р±СѓРґРµС‚ РґРѕСЃС‚СѓРїРµРЅ
        T result = results[id]; // РџРѕР»СѓС‡Р°РµРј СЂРµР·СѓР»СЊС‚Р°С‚ РІС‹РїРѕР»РЅРµРЅРёСЏ Р·Р°РґР°С‡Рё
        results.erase(id);// РЈРґР°Р»СЏРµРј СЂРµР·СѓР»СЊС‚Р°С‚ РёР· РєРѕРЅС‚РµР№РЅРµСЂР°
        return result;
    }

private:
    std::mutex mutex_;// РњСЊСЋС‚РµРєСЃ РґР»СЏ СЃРёРЅС…СЂРѕРЅРёР·Р°С†РёРё РґРѕСЃС‚СѓРїР° Рє РґР°РЅРЅС‹Рј
    std::condition_variable cv;// РЈСЃР»РѕРІРЅР°СЏ РїРµСЂРµРјРµРЅРЅР°СЏ РґР»СЏ РѕР¶РёРґР°РЅРёСЏ СЃРѕР±С‹С‚РёР№
    std::thread serverThread;
    bool stopFlag = false;// Р¤Р»Р°Рі РѕСЃС‚Р°РЅРѕРІРєРё СЃРµСЂРІРµСЂР°
    size_t id = 1;// РРґРµРЅС‚РёС„РёРєР°С‚РѕСЂ Р·Р°РґР°С‡Рё
    std::queue<std::pair<size_t, std::future<T>>> tasks;// РћС‡РµСЂРµРґСЊ
    std::unordered_map<size_t, T> results; // РљРѕРЅС‚РµР№РЅРµСЂ РґР»СЏ С…СЂР°РЅРµРЅРёСЏ СЂРµР·СѓР»СЊС‚Р°С‚РѕРІ РІС‹РїРѕР»РЅРµРЅРёСЏ Р·Р°РґР°С‡

    void serverThreadFunction() {  // Р¤СѓРЅРєС†РёСЏ РїРѕС‚РѕРєР° РґР»СЏ РѕР±СЂР°Р±РѕС‚РєРё Р·Р°РґР°С‡
        while (true) {
            std::unique_lock<std::mutex> lock(mutex_);
            cv.wait(lock, [this] { return !tasks.empty() || stopFlag; }); // РћР¶РёРґР°РµРј РїРѕСЏРІР»РµРЅРёСЏ РЅРѕРІРѕР№ Р·Р°РґР°С‡Рё РёР»Рё СЃРёРіРЅР°Р»Р° РѕР± РѕСЃС‚Р°РЅРѕРІРєРµ СЃРµСЂРІРµСЂР°

            if (tasks.empty() && stopFlag) {  // Р•СЃР»Рё РѕС‡РµСЂРµРґСЊ РїСѓСЃС‚Р° Рё СЃРµСЂРІРµСЂ РґРѕР»Р¶РµРЅ Р±С‹С‚СЊ РѕСЃС‚Р°РЅРѕРІР»РµРЅ, РІС‹С…РѕРґРёРј РёР· С†РёРєР»Р°
                break;
            }

            if (!tasks.empty()) { // Р•СЃР»Рё РѕС‡РµСЂРµРґСЊ РЅРµ РїСѓСЃС‚Р°, РёР·РІР»РµРєР°РµРј Р·Р°РґР°С‡Сѓ Рё РІС‹РїРѕР»РЅСЏРµРј РµРµ
                auto task = std::move(tasks.front());// РР·РІР»РµРєР°РµРј Р·Р°РґР°С‡Сѓ РёР· РѕС‡РµСЂРµРґРё
                tasks.pop();// РЈРґР°Р»СЏРµРј Р·Р°РґР°С‡Сѓ РёР· РѕС‡РµСЂРµРґРё
                results[task.first] = task.second.get();// РџРѕР»СѓС‡Р°РµРј СЂРµР·СѓР»СЊС‚Р°С‚ РІС‹РїРѕР»РЅРµРЅРёСЏ Р·Р°РґР°С‡Рё Рё СЃРѕС…СЂР°РЅСЏРµРј РµРіРѕ
                cv.notify_one();// РЈРІРµРґРѕРјР»СЏРµРј Рѕ РЅР°Р»РёС‡РёРё СЂРµР·СѓР»СЊС‚Р°С‚Р°
            }
        }
    }
};

template <typename T>
class CustomTaskClient {
public:
    void runClient(CustomTaskServer<T>& server, std::function<std::pair<T, T>()> taskGenerator) { // РњРµС‚РѕРґ РґР»СЏ Р·Р°РїСѓСЃРєР° РєР»РёРµРЅС‚Р°
        auto task = taskGenerator();// Р“РµРЅРµСЂРёСЂСѓРµРј Р·Р°РґР°С‡Сѓ
        int id = server.addTaskQueue([task]() { return task.second; });// Р”РѕР±Р°РІР»СЏРµРј Р·Р°РґР°С‡Сѓ РІ РѕС‡РµСЂРµРґСЊ РЅР° СЃРµСЂРІРµСЂРµ
        taskIds.push_back({ id, task.first });// РЎРѕС…СЂР°РЅСЏРµРј РёРґРµРЅС‚РёС„РёРєР°С‚РѕСЂ Р·Р°РґР°С‡Рё
    }

    std::list<std::pair<T, T>> clientToResult(CustomTaskServer<T>& server) // РњРµС‚РѕРґ РґР»СЏ РїРѕР»СѓС‡РµРЅРёСЏ СЂРµР·СѓР»СЊС‚Р°С‚РѕРІ РІС‹РїРѕР»РЅРµРЅРёСЏ Р·Р°РґР°С‡ РєР»РёРµРЅС‚РѕРј
    {
        std::list<std::pair<T, T>> results;// РљРѕРЅС‚РµР№РЅРµСЂ РґР»СЏ СЂРµР·СѓР»СЊС‚Р°С‚РѕРІ
        for (const auto& taskId : taskIds) {// Р”Р»СЏ РєР°Р¶РґРѕРіРѕ РёРґРµРЅС‚РёС„РёРєР°С‚РѕСЂР° Р·Р°РґР°С‡Рё Р·Р°РїСЂР°С€РёРІР°РµРј СЂРµР·СѓР»СЊС‚Р°С‚ РІС‹РїРѕР»РЅРµРЅРёСЏ
            T result = server.requestResult(taskId.first);// Р—Р°РїСЂР°С€РёРІР°РµРј СЂРµР·СѓР»СЊС‚Р°С‚
            results.push_back(std::make_pair(taskId.first, result));// РЎРѕС…СЂР°РЅСЏРµРј СЂРµР·СѓР»СЊС‚Р°С‚
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

// Р—Р°РґР°С‡Р° 2: Р’С‹С‡РёСЃР»РµРЅРёРµ РєРѕСЂРЅСЏ РёР· СЃР»СѓС‡Р°Р№РЅРѕРіРѕ С‡РёСЃР»Р° 
template<typename T>
std::pair<T, T> randomSqrt() {
    static std::default_random_engine generator;
    static std::uniform_real_distribution<T> distribution(1.0, 10.0);
    T value = distribution(generator);
    return { value, std::sqrt(value) };
}

// Р—Р°РґР°С‡Р° 3: Р’С‹С‡РёСЃР»РµРЅРёРµ РєРІР°РґСЂР°С‚Р° СЃР»СѓС‡Р°Р№РЅРѕРіРѕ С‡РёСЃР»Р° 
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

    // РЎРѕС…СЂР°РЅРµРЅРёРµ СЂРµР·СѓР»СЊС‚Р°С‚РѕРІ РІ С„Р°Р№Р»С‹
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
