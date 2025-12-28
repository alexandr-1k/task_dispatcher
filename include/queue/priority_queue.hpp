#pragma once
#include "queue/bounded_queue.hpp"
#include "queue/unbounded_queue.hpp"
#include "types.hpp"
#include <condition_variable>
#include <map>
#include <memory>
#include <optional>

namespace dispatcher::queue {

class PriorityQueue {
    const std::map<dispatcher::TaskPriority, std::unique_ptr<IQueue>> queues_;
    std::mutex mx_;
    std::condition_variable cv_;
    std::atomic<bool> stopping_{false};
    std::atomic<size_t> tasks_count_{0};

public:
    explicit PriorityQueue(std::map<dispatcher::TaskPriority, QueueOptions> options);

    void push(TaskPriority priority, std::function<void()> task);
    // block on pop until shutdown is called
    // after that return std::nullopt on empty queue
    std::optional<std::function<void()>> pop();

    void shutdown();

    ~PriorityQueue();
};

}  // namespace dispatcher::queue