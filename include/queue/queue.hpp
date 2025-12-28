#pragma once
#include <atomic>
#include <functional>
#include <optional>

namespace dispatcher::queue {

struct QueueOptions {
    bool bounded;
    std::optional<int> capacity;
};

class IQueue {
public:
    virtual ~IQueue() = default;
    virtual void push(std::function<void()> task) = 0;
    virtual std::optional<std::function<void()>> try_pop() = 0;
};

class SpinLock {
    std::atomic_flag &lock_;

public:
    SpinLock(std::atomic_flag &flag) : lock_(flag) { lock(); }
    ~SpinLock() { unlock(); }

    void lock() {
        // possibly a little better implementation of spinlock
        // https://rigtorp.se/spinlock
        while (true) {
            if (!lock_.test_and_set(std::memory_order_acquire)) {
                return;  // optimistic: we got the lock immediately
            }
            while (lock_.test(std::memory_order_relaxed))
                ;
        }
    }

    void unlock() { lock_.clear(std::memory_order_release); }

    bool is_locked() const { return lock_.test(std::memory_order_relaxed); }

    bool try_lock() { return !lock_.test_and_set(std::memory_order_acquire); }
};

}  // namespace dispatcher::queue