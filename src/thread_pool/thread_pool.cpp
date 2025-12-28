#include "thread_pool/thread_pool.hpp"
#include <stop_token>

namespace dispatcher::thread_pool {

ThreadPool::ThreadPool(std::shared_ptr<queue::PriorityQueue> pq, size_t num_threads) : pq_(std::move(pq)) {
    workers_.reserve(num_threads);
    for (size_t i = 0; i < num_threads; ++i) {
        workers_.emplace_back([this](std::stop_token stoken) { worker_thread(stoken); });
    }
}

ThreadPool::~ThreadPool() {
    pq_->shutdown();
    for (auto &w : workers_) {
        w.request_stop();
    }
}

void ThreadPool::worker_thread(std::stop_token stoken) {
    while (true) {
        auto task = pq_->pop();  // blocks if no tasks are in the queue
        if (task.has_value()) {
            (*task)();
        } else if (stoken.stop_requested()) {
            break;
        } else {
            std::this_thread::yield();
        }
    };
}
}  // namespace dispatcher::thread_pool