#include <gtest/gtest.h>

#include "task_dispatcher.hpp"

TEST(TestTaskDispatcher, HighPriorityTasksCompleteBeforeDestruction) {
    const int total_tasks = 10;
    int counter = 0;
    {
        dispatcher::TaskDispatcher dispatcher(
            4, {
                   {dispatcher::TaskPriority::High, dispatcher::queue::QueueOptions{.bounded = true, .capacity = 10}},
                   {dispatcher::TaskPriority::Normal, dispatcher::queue::QueueOptions{.bounded = false}},
               });

        for (int i = 0; i < total_tasks; ++i) {
            dispatcher.schedule(dispatcher::TaskPriority::High, [&counter]() {
                ++counter;
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            });
        }
    }

    EXPECT_EQ(counter, total_tasks);
}

TEST(TestTaskDispatcher, NormalPriorityTasksMayNotCompleteBeforeDestruction) {
    const int total_tasks = 10;
    int counter = 0;
    {
        dispatcher::TaskDispatcher dispatcher(
            4, {
                   {dispatcher::TaskPriority::High, dispatcher::queue::QueueOptions{.bounded = true, .capacity = 10}},
                   {dispatcher::TaskPriority::Normal, dispatcher::queue::QueueOptions{.bounded = false}},
               });

        for (int i = 0; i < total_tasks; ++i) {
            dispatcher.schedule(dispatcher::TaskPriority::Normal, [&counter]() {
                ++counter;
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            });
        }
    }

    EXPECT_LT(counter, total_tasks);
}

TEST(TestTaskDispatcher, MixedPriorityTasksCompleteHighBeforeNormal) {
    const int high_priority_tasks = 5;
    const int normal_priority_tasks = 5;
    int high_counter = 0;
    int normal_counter = 0;
    {
        dispatcher::TaskDispatcher dispatcher(
            4, {
                   {dispatcher::TaskPriority::High, dispatcher::queue::QueueOptions{.bounded = true, .capacity = 10}},
                   {dispatcher::TaskPriority::Normal, dispatcher::queue::QueueOptions{.bounded = false}},
               });

        for (int i = 0; i < normal_priority_tasks; ++i) {
            dispatcher.schedule(dispatcher::TaskPriority::Normal, [&normal_counter]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(300));
                ++normal_counter;
            });
        }

        for (int i = 0; i < high_priority_tasks; ++i) {
            dispatcher.schedule(dispatcher::TaskPriority::High, [&high_counter]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                ++high_counter;
            });
        }
    }

    EXPECT_EQ(high_counter, high_priority_tasks);
    EXPECT_LT(normal_counter, normal_priority_tasks);
}

TEST(TestTaskDispatcher, NoTasksScheduled) {
    {
        dispatcher::TaskDispatcher dispatcher(
            4, {
                   {dispatcher::TaskPriority::High, dispatcher::queue::QueueOptions{.bounded = true, .capacity = 10}},
                   {dispatcher::TaskPriority::Normal, dispatcher::queue::QueueOptions{.bounded = false}},
               });
    }
    SUCCEED();  // no need to block if empty
}

TEST(TestTaskDispatcher, InvalidParams) {
    EXPECT_THROW(
        {
            dispatcher::TaskDispatcher dispatcher(
                4, {
                       {dispatcher::TaskPriority::High, dispatcher::queue::QueueOptions{.bounded = true}},
                       {dispatcher::TaskPriority::Normal, dispatcher::queue::QueueOptions{.bounded = false}},
                   });
        },
        std::invalid_argument);
}

// здесь ваш код