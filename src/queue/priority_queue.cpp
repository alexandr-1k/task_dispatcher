#include "queue/priority_queue.hpp"
#include "queue/queue.hpp"
#include <algorithm>
#include <atomic>
#include <optional>

namespace dispatcher::queue {

PriorityQueue::PriorityQueue(std::map<dispatcher::TaskPriority, QueueOptions> options)
    : queues_([&] {  // to keep `queues_` const
          std::map<TaskPriority, std::unique_ptr<IQueue>> m;
          for (const auto &[priority, option] : options) {
              if (option.bounded) {
                  if (!option.capacity.has_value()) {
                      throw std::invalid_argument("Bounded queue must have capacity");
                  }
                  m.emplace(priority, std::make_unique<BoundedQueue>(option.capacity.value()));
              } else {
                  m.emplace(priority, std::make_unique<UnboundedQueue>());
              }
          }
          return m;
      }()) {}

PriorityQueue::~PriorityQueue() { shutdown(); }

void PriorityQueue::push(TaskPriority priority, std::function<void()> task) {
    auto it = queues_.find(priority);
    if (it == queues_.end()) {
        throw std::invalid_argument("Invalid priority");
    }

    std::unique_lock<std::mutex> guard(mx_);
    if (stopping_.load(std::memory_order_acquire)) {
        throw std::runtime_error("Queue is shutting down");
    }
    it->second->push(std::move(task));
    tasks_count_.fetch_add(1, std::memory_order_release);
    cv_.notify_one();
}

void PriorityQueue::shutdown() {
    std::unique_lock<std::mutex> guard(mx_);
    stopping_.store(true, std::memory_order_release);
    cv_.notify_all();
}

std::optional<std::function<void()>> PriorityQueue::pop() {
    std::unique_lock<std::mutex> guard(mx_);
    cv_.wait(guard, [this] {
        return stopping_.load(std::memory_order_acquire) || tasks_count_.load(std::memory_order_acquire) > 0;
    });

    auto stopping = stopping_.load(std::memory_order_acquire);

    if (stopping_) {
        auto highest_priority_it = std::min_element(queues_.begin(), queues_.end());
        if (highest_priority_it == queues_.end()) {
            return std::nullopt;
        }
        auto &queue = highest_priority_it->second;

        auto task = queue->try_pop();
        if (!task.has_value()) {
            return std::nullopt;
        }
        tasks_count_.fetch_sub(1, std::memory_order_release);
        return task;
    }

    for (auto &[priority, queue] : queues_) {
        auto task = queue->try_pop();
        if (task.has_value()) {
            tasks_count_.fetch_sub(1, std::memory_order_release);
            return task;
        }
    }

    return std::nullopt;
}

}  // namespace dispatcher::queue