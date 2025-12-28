#include <gtest/gtest.h>

#include "queue/unbounded_queue.hpp"

TEST(UnboundedQueueTests, SimplePushPop) {
    dispatcher::queue::UnboundedQueue queue;

    int some_value = 10;

    queue.push([&some_value] { some_value = some_value * 3; });

    auto task1 = queue.try_pop();
    ASSERT_TRUE(task1.has_value());
    (*task1)();
    ASSERT_EQ(some_value, 30);
}

TEST(UnboundedQueueTests, MultiplePushPop) {
    dispatcher::queue::UnboundedQueue queue;

    int some_value = 2;

    queue.push([&some_value] { some_value += 5; });
    queue.push([&some_value] { some_value *= 3; });
    queue.push([&some_value] { some_value -= 4; });

    auto task1 = queue.try_pop();
    ASSERT_TRUE(task1.has_value());
    (*task1)();
    ASSERT_EQ(some_value, 7);  // 2 + 5

    auto task2 = queue.try_pop();
    ASSERT_TRUE(task2.has_value());
    (*task2)();
    ASSERT_EQ(some_value, 21);  // 7 * 3

    auto task3 = queue.try_pop();
    ASSERT_TRUE(task3.has_value());
    (*task3)();
    ASSERT_EQ(some_value, 17);  // 21 - 4
}

TEST(UnboundedQueueTests, TryPopEmptyQueue) {
    dispatcher::queue::UnboundedQueue queue;

    auto task = queue.try_pop();
    ASSERT_FALSE(task.has_value());  // Queue is empty
}

TEST(UnboundedQueueTests, HighVolumePushPop) {
    dispatcher::queue::UnboundedQueue queue;

    const int num_tasks = 1000;
    int counter = 0;

    // Push a large number of tasks
    for (int i = 0; i < num_tasks; ++i) {
        queue.push([&counter] { counter++; });
    }

    // Pop and execute all tasks
    for (int i = 0; i < num_tasks; ++i) {
        auto task = queue.try_pop();
        ASSERT_TRUE(task.has_value());
        (*task)();
    }

    ASSERT_EQ(counter, num_tasks);
}

// check if push/pop are serialized
TEST(UnboundedQueueTests, OverlappingPushPopInThreads) {
    dispatcher::queue::UnboundedQueue queue;

    const int num_tasks = 500;
    int counter = 0;

    // Producer thread
    std::thread producer([&queue, &counter, num_tasks]() {
        for (int i = 0; i < num_tasks; ++i) {
            queue.push([&counter] { ++counter; });
        }
    });

    // Consumer thread
    std::thread consumer([&queue, num_tasks]() {
        int local_count = 0;
        while (local_count < num_tasks) {
            auto task = queue.try_pop();
            if (task.has_value()) {
                (*task)();
                local_count++;
            } else {
                std::this_thread::yield();  // Avoid busy waiting
            }
        }
    });

    producer.join();
    consumer.join();

    ASSERT_EQ(counter, num_tasks);
}

// здесь ваш код