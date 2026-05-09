#include "threadpool.hpp"
#include <iostream>

ThreadPool::ThreadPool(int num_threads) {
    for (int i = 0; i < num_threads; i++) {
        // Each worker waits for a client_fd to appear in the queue
        workers.emplace_back([this]() {
            while (true) {
                int client_fd;
                {
                    std::unique_lock<std::mutex> lock(queue_mutex);
                    // Sleep until there's work or the pool is shutting down
                    cv.wait(lock, [this]() { return !task_queue.empty() || stop; });
                    if (stop && task_queue.empty()) return;
                    client_fd = task_queue.front();
                    task_queue.pop();
                }
                // Handle the client outside the lock so other workers can pick up tasks
                if (client_handler) client_handler(client_fd);
            }
        });
    }
}

ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }
    cv.notify_all();  // wake all workers so they can exit
    for (auto& t : workers) t.join();
}

void ThreadPool::enqueue(int client_fd) {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        task_queue.push(client_fd);
    }
    cv.notify_one();  // wake one idle worker
}
