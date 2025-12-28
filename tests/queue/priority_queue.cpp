#include <gtest/gtest.h>

#include "queue/priority_queue.hpp"

struct Task {
    dispatcher::TaskPriority priority;
    int id;
};
void PutSomeTasks(std::vector<Task> &processed_tasks, dispatcher::queue::PriorityQueue &pq, int &counter);

TEST(PriorityQueueTest, HighPriorityTasksPopedFirst) {
    dispatcher::queue::PriorityQueue pq{{
        {dispatcher::TaskPriority::High, dispatcher::queue::QueueOptions{.bounded = true, .capacity = 10}},
        {dispatcher::TaskPriority::Normal, dispatcher::queue::QueueOptions{.bounded = false}},
    }};

    std::vector<Task> processed_tasks;
    int counter = 0;

    PutSomeTasks(processed_tasks, pq, counter);

    // execute all
    for (int i = 0; i < 10; ++i) {
        auto task = pq.pop();
        if (task) {
            task.value()();
        }
    }

    // Verify that high priority tasks were processed first
    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ(processed_tasks[i].priority, dispatcher::TaskPriority::High);
    }
    for (int i = 5; i < 10; ++i) {
        EXPECT_EQ(processed_tasks[i].priority, dispatcher::TaskPriority::Normal);
    }
}

TEST(PriorityQueueTest, PopBlocksUntilTaskIsPushed) {
    dispatcher::queue::PriorityQueue pq{{
        {dispatcher::TaskPriority::Normal, dispatcher::queue::QueueOptions{.bounded = false}},
    }};

    bool task_executed = false;

    auto task_executor = [&task_executed]() { task_executed = true; };

    std::thread pop_thread([&pq]() {
        auto task = pq.pop();
        if (task) {
            task.value()();
        }
    });

    // Ensure pop_thread is likely waiting
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_FALSE(task_executed);

    // Now push a task
    pq.push(dispatcher::TaskPriority::Normal, task_executor);

    pop_thread.join();

    EXPECT_TRUE(task_executed);
}

TEST(PriorityQueueTest, OnShutdownOnlyHighPriorityTasksAreProcessed) {
    dispatcher::queue::PriorityQueue pq{{
        {dispatcher::TaskPriority::High, dispatcher::queue::QueueOptions{.bounded = true, .capacity = 10}},
        {dispatcher::TaskPriority::Normal, dispatcher::queue::QueueOptions{.bounded = false}},
    }};

    std::vector<Task> processed_tasks;
    int counter = 0;

    PutSomeTasks(processed_tasks, pq, counter);

    // Shutdown the queue
    pq.shutdown();

    // execute all
    for (int i = 0; i < 10; ++i) {
        auto task = pq.pop();
        if (task) {
            task.value()();
        }
    }

    // Verify that on shutdown only high priority tasks were processed
    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ(processed_tasks[i].priority, dispatcher::TaskPriority::High);
    }
    EXPECT_EQ(processed_tasks.size(), 5);
}

TEST(PriorityQueueTest, PushPopSerialized) {
    dispatcher::queue::PriorityQueue pq{{
        {dispatcher::TaskPriority::Normal, dispatcher::queue::QueueOptions{.bounded = false}},
    }};

    const int num_tasks = 100;
    int counter = 0;

    auto task_executor = [&counter]() { ++counter; };

    // Producer thread
    std::thread producer([&pq, &task_executor, num_tasks]() {
        for (int i = 0; i < num_tasks; ++i) {
            pq.push(dispatcher::TaskPriority::Normal, task_executor);
        }
    });

    // Consumer thread
    std::thread consumer([&pq, num_tasks]() {
        int local_count = 0;
        while (local_count < num_tasks) {
            auto task = pq.pop();
            if (task) {
                task.value()();
                local_count++;
            } else {
                std::this_thread::yield();  // Avoid busy waiting
            }
        }
    });

    producer.join();
    consumer.join();
    EXPECT_EQ(counter, num_tasks);
}

TEST(PriorityQueueTest, PushAfterShutdownThrows) {
    dispatcher::queue::PriorityQueue pq{{
        {dispatcher::TaskPriority::Normal, dispatcher::queue::QueueOptions{.bounded = false}},
    }};

    pq.shutdown();

    auto task_executor = []() {};

    EXPECT_THROW(pq.push(dispatcher::TaskPriority::Normal, task_executor), std::runtime_error);
}

void PutSomeTasks(std::vector<Task> &processed_tasks, dispatcher::queue::PriorityQueue &pq, int &counter) {
    auto high_priority_executor = [&processed_tasks, &counter]() {
        processed_tasks.push_back({dispatcher::TaskPriority::High, counter++});
    };

    auto normal_priority_executor = [&processed_tasks, &counter]() {
        processed_tasks.push_back({dispatcher::TaskPriority::Normal, counter++});
    };

    // Schedule normal priority tasks
    for (int i = 0; i < 2; ++i) {
        pq.push(dispatcher::TaskPriority::Normal, normal_priority_executor);
    }

    // Schedule high priority tasks
    for (int i = 0; i < 5; ++i) {
        pq.push(dispatcher::TaskPriority::High, high_priority_executor);
    }

    // Schedule more normal priority tasks
    for (int i = 0; i < 3; ++i) {
        pq.push(dispatcher::TaskPriority::Normal, normal_priority_executor);
    }
}
// здесь ваш код