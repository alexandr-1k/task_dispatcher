#include <gtest/gtest.h>

#include "queue/bounded_queue.hpp"

TEST(BoundedQueueTests, SimplePushPop) {
    dispatcher::queue::BoundedQueue queue(1);

    int some_value = 10;

    queue.push([&some_value] { some_value = some_value * 3; });

    auto task1 = queue.try_pop();
    ASSERT_TRUE(task1.has_value());
    (*task1)();
    ASSERT_EQ(some_value, 30);
}

TEST(BoundedQueueTests, BoundedBehavior) {
    dispatcher::queue::BoundedQueue queue(3);

    int some_value = 5;

    queue.push([&some_value] { some_value += 2; });
    queue.push([&some_value] { some_value *= 4; });

    // Queue is full now; next push should block until a pop occurs
    std::thread producer([&queue, &some_value]() { queue.push([&some_value] { some_value -= 3; }); });

    // Allow some time for the producer to potentially block
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    ASSERT_EQ(some_value, 5);  // Value should remain unchanged

    // Pop one task to unblock the producer
    auto task1 = queue.try_pop();
    ASSERT_TRUE(task1.has_value());
    (*task1)();
    ASSERT_EQ(some_value, 7);  // 5 + 2

    producer.join();  // Ensure producer thread has finished

    // Now pop the remaining tasks
    auto task2 = queue.try_pop();
    ASSERT_TRUE(task2.has_value());
    (*task2)();
    ASSERT_EQ(some_value, 28);  // 7 * 4

    auto task3 = queue.try_pop();
    ASSERT_TRUE(task3.has_value());
    (*task3)();
    ASSERT_EQ(some_value, 25);  // 28 - 3
}

TEST(BoundedQueueTests, TryPopEmptyQueue) {
    dispatcher::queue::BoundedQueue queue(2);

    auto task = queue.try_pop();
    ASSERT_FALSE(task.has_value());  // Queue is empty
}

TEST(BoundedQueueTests, PushToFullQueueThrows) {
    dispatcher::queue::BoundedQueue queue(1);

    queue.push([] {});

    // Next push should throw since the queue is full
    ASSERT_THROW(queue.push([] {}), std::runtime_error);
}

TEST(BoundedQueueTests, InvalidCapacityThrows) {
    // Capacity must be positive
    ASSERT_THROW(dispatcher::queue::BoundedQueue queue(0), std::invalid_argument);
    ASSERT_THROW(dispatcher::queue::BoundedQueue queue(-5), std::invalid_argument);
}

TEST(BoundedQueueTests, CircularBuffer) {
    dispatcher::queue::BoundedQueue queue(2);

    int counter = 0;

    queue.push([&counter] { counter += 1; });
    queue.push([&counter] { counter += 2; });

    auto task1 = queue.try_pop();
    ASSERT_TRUE(task1.has_value());
    (*task1)();
    ASSERT_EQ(counter, 1);

    queue.push([&counter] { counter += 3; });

    auto task2 = queue.try_pop();
    ASSERT_TRUE(task2.has_value());
    (*task2)();
    ASSERT_EQ(counter, 3);  // 1 + 2

    auto task3 = queue.try_pop();
    ASSERT_TRUE(task3.has_value());
    (*task3)();
    ASSERT_EQ(counter, 6);  // 3 + 3
}

// check if push/pop are serialized
TEST(BoundedQueueTests, OverlappingPushPopInThreads) {
    dispatcher::queue::BoundedQueue queue(500);

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