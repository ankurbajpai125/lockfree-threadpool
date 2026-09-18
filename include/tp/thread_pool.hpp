#pragma once
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <vector>

namespace tp {

class ThreadPool {
public:
    explicit ThreadPool(std::size_t n = std::thread::hardware_concurrency()) {
        if (n == 0) n = 1;
        workers_.reserve(n);
        for (std::size_t i = 0; i < n; ++i)
            workers_.emplace_back([this] { worker_loop(); });
    }

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    ~ThreadPool() { shutdown(); }

    // Pass reference arguments with std::ref (same rule as std::thread/std::bind).
    template <class F, class... Args>
    auto submit(F&& f, Args&&... args)
        -> std::future<std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>> {
        using R = std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>;

        // packaged_task is move-only, std::function needs a copyable callable,
        // so the task is wrapped in a shared_ptr.
        auto task = std::make_shared<std::packaged_task<R()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));
        std::future<R> fut = task->get_future();
        {
            std::lock_guard<std::mutex> lk(m_);
            if (stop_) throw std::runtime_error("submit() on a stopped ThreadPool");
            tasks_.emplace([task] { (*task)(); });
        }
        cv_.notify_one();  // notify after releasing the lock
        return fut;
    }

private:
    void worker_loop() {
        for (;;) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lk(m_);
                // The predicate guards against spurious wakeups.
                cv_.wait(lk, [this] { return stop_ || !tasks_.empty(); });
                if (stop_ && tasks_.empty()) return;  // drain the queue before exiting
                task = std::move(tasks_.front());
                tasks_.pop();
            }
            task();  // run outside the lock
        }
    }

    void shutdown() {
        {
            std::lock_guard<std::mutex> lk(m_);
            if (stop_) return;
            stop_ = true;
        }
        cv_.notify_all();
        for (auto& t : workers_)
            if (t.joinable()) t.join();
    }

    std::mutex m_;
    std::condition_variable cv_;
    std::queue<std::function<void()>> tasks_;
    bool stop_ = false;
    std::vector<std::thread> workers_;
};

}  // namespace tp
