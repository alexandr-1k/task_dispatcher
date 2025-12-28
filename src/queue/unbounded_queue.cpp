#include "queue/unbounded_queue.hpp"
#include "queue/queue.hpp"

#include <functional>

namespace dispatcher::queue {

UnboundedQueue::UnboundedQueue() = default;

UnboundedQueue::~UnboundedQueue() = default;

void UnboundedQueue::push(std::function<void()> task) {
    SpinLock lk{lock_};
    buffer_.push_back(std::move(task));
}

std::optional<std::function<void()>> UnboundedQueue::try_pop() {
    SpinLock lk{lock_};
    if (buffer_.empty()) {
        return std::nullopt;
    }
    auto task = std::move(buffer_.front());
    buffer_.pop_front();
    return task;
}

}  // namespace dispatcher::queue