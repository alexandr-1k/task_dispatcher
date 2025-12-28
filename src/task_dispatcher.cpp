#include "task_dispatcher.hpp"

namespace dispatcher {

TaskDispatcher::TaskDispatcher(size_t num_threads, std::map<TaskPriority, queue::QueueOptions> queue_options)
    : pq_(std::make_shared<queue::PriorityQueue>(std::move(queue_options))), thread_pool_(pq_, num_threads) {}

void TaskDispatcher::schedule(TaskPriority priority, std::function<void()> task) {
    pq_->push(priority, std::move(task));
}

TaskDispatcher::~TaskDispatcher() = default;
}  // namespace dispatcher