#include "thread_pool/thread_pool.hpp"

namespace dispatcher::thread_pool {

ThreadPool::ThreadPool(std::shared_ptr<queue::PriorityQueue> pq, size_t num_threads) : pq_(std::move(pq)) {
    for (size_t i = 0; i < num_threads; ++i) {
        workers_.emplace_back(&ThreadPool::worker_thread, this);
    }
}

ThreadPool::~ThreadPool() {
    pq_->shutdown();
    stopping_.store(true, std::memory_order_release);
    for (auto &th : workers_) {
        if (th.joinable()) {
            th.join();
        }
    }
}

void ThreadPool::worker_thread() {
    while (true) {
        auto task = pq_->pop();  // blocks if no tasks are in the queue
        if (task.has_value()) {
            (*task)();
        } else if (stopping_.load(std::memory_order_acquire)) {
            break;
        } else {
            std::this_thread::yield();
        }
    };
}
}  // namespace dispatcher::thread_pool