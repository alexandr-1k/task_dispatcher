#pragma once

#include <memory>

#include "queue/priority_queue.hpp"
#include "queue/queue.hpp"
#include "thread_pool/thread_pool.hpp"
#include "types.hpp"

namespace dispatcher {

class TaskDispatcher {
    std::shared_ptr<queue::PriorityQueue> pq_;
    thread_pool::ThreadPool thread_pool_;

public:
    TaskDispatcher(size_t, std::map<TaskPriority, queue::QueueOptions> = {
                               {TaskPriority::High, queue::QueueOptions{.bounded = true, .capacity = 1000}},
                               {TaskPriority::Normal, queue::QueueOptions{.bounded = false}},
                           });

    void schedule(TaskPriority priority, std::function<void()> task);
    ~TaskDispatcher();
};

}  // namespace dispatcher