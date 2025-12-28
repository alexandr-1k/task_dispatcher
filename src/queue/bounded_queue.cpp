#include "queue/bounded_queue.hpp"
#include "queue/queue.hpp"
#include <stdexcept>

namespace dispatcher::queue {

BoundedQueue::BoundedQueue(int capacity) : buffer_(capacity), capacity_(capacity) {
    if (capacity <= 0) {
        throw std::invalid_argument("Capacity must be positive");
    }
}

BoundedQueue::~BoundedQueue() = default;

void BoundedQueue::push(std::function<void()> task) {
    SpinLock lk{lock_};
    if (size_ == capacity_) {
        throw std::runtime_error("BoundedQueue is full");
    }
    auto index = (tail_ + 1) % capacity_;
    buffer_[index] = std::move(task);
    tail_ = index;
    ++size_;
}

std::optional<std::function<void()>> BoundedQueue::try_pop() {
    SpinLock lk{lock_};
    if (size_ == 0) {
        return std::nullopt;
    }
    auto index = (head_ + 1) % capacity_;
    auto task = std::move(buffer_[index]);
    head_ = index;
    --size_;
    return task;
}
}  // namespace dispatcher::queue