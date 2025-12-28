#pragma once
#include "queue/queue.hpp"
#include <atomic>
#include <deque>

namespace dispatcher::queue {

class UnboundedQueue : public IQueue {
    std::deque<std::function<void()>> buffer_;
    std::atomic_flag lock_{false};

public:
    explicit UnboundedQueue();

    void push(std::function<void()> task) override;

    std::optional<std::function<void()>> try_pop() override;

    ~UnboundedQueue() override;
};

}  // namespace dispatcher::queue