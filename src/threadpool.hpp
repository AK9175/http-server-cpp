#pragma once
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>

class ThreadPool {
public:
    ThreadPool(int num_threads);
    ~ThreadPool();
    void enqueue(int client_fd);

private:
    std::vector<std::thread> workers;
    std::queue<int> task_queue;
    std::mutex queue_mutex;
    std::condition_variable cv;
    bool stop = false;
    std::function<void(int)> handler;

public:
    std::function<void(int)> client_handler;
};
