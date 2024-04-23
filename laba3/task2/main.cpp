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