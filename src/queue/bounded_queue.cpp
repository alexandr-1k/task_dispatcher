#include "queue/bounded_queue.hpp"
#include "queue/queue.hpp"
// #include <new>
#include <stdexcept>

namespace dispatcher::queue {

BoundedQueue::BoundedQueue(int capacity)
    : buffer_([capacity] {
          if (capacity <= 0) {
              throw std::invalid_argument("Capacity must be positive");
          }
          std::vector<std::function<void()>> buf;
          buf.reserve(capacity);
          return buf;
      }()),
      capacity_(capacity) {}

BoundedQueue::~BoundedQueue() = default;

void BoundedQueue::push(std::function<void()> task) {
    SpinLock lk{lock_};
    if (size_ == capacity_) {
        throw std::runtime_error("BoundedQueue is full");
    }
    auto index = (tail_ + 1) % capacity_;
    std::construct_at(buffer_.data() + index, std::move(task));
    tail_ = index;
    ++size_;
}

std::optional<std::function<void()>> BoundedQueue::try_pop() {
    SpinLock lk{lock_};
    if (size_ == 0) {
        return std::nullopt;
    }
    auto index = (head_ + 1) % capacity_;
    auto task = std::move(*(buffer_.data() + index));
    std::destroy_at(buffer_.data() + index);
    head_ = index;
    --size_;
    return task;
}
}  // namespace dispatcher::queue