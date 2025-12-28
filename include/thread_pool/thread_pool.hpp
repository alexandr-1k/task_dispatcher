#pragma once

#include "queue/priority_queue.hpp"
#include <thread>
#include <vector>

namespace dispatcher::thread_pool {

class ThreadPool {
    std::shared_ptr<queue::PriorityQueue> pq_;
    std::vector<std::jthread> workers_;

public:
    explicit ThreadPool(std::shared_ptr<queue::PriorityQueue>, size_t);
    ~ThreadPool();

private:
    void worker_thread(std::stop_token);
};

}  // namespace dispatcher::thread_pool
