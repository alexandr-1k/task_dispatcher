#pragma once
#include "queue/queue.hpp"
#include <cstddef>
#include <vector>

namespace dispatcher::queue {

class BoundedQueue : public IQueue {
    std::vector<std::function<void()>> buffer_;
    std::atomic_flag lock_{false};
    const size_t capacity_;
    size_t head_{0};
    size_t tail_{0};
    size_t size_{0};

public:
    explicit BoundedQueue(int capacity);

    void push(std::function<void()> task) override;

    std::optional<std::function<void()>> try_pop() override;

    ~BoundedQueue() override;
};

}  // namespace dispatcher::queue