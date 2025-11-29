#include <iostream>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <atomic>

class ThreadPool {
public:
    // 构造函数，创建指定数量的工作线程
    ThreadPool(size_t numThreads) : stop(false) {
        for (size_t i = 0; i < numThreads; ++i) {
            workers.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    
                    {
                        // 等待任务或停止信号
                        std::unique_lock<std::mutex> lock(queueMutex);
                        condition.wait(lock, [this] {
                            return stop || !tasks.empty();
                        });

                        if (stop && tasks.empty()) {
                            return;  // 线程池停止且任务队列为空，退出线程
                        }

                        task = std::move(tasks.front());
                        tasks.pop();
                    }
                    task();  // 执行任务
                }
            });
        }
    }

    // 禁止拷贝构造和赋值操作
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    // 提交任务到线程池并返回future对象
    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type> {
        using return_type = typename std::result_of<F(Args...)>::type;

        // 创建packaged_task来包装任务
        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        std::future<return_type> result = task->get_future();
        
        {
            std::unique_lock<std::mutex> lock(queueMutex);

            // 如果线程池已停止，抛出异常
            if (stop) {
                throw std::runtime_error("enqueue on stopped ThreadPool");
            }

            // 将任务添加到队列
            tasks.emplace([task]() { (*task)(); });
        }

        condition.notify_one();  // 通知一个等待中的线程
        return result;
    }

    // 获取线程池中的线程数量
    size_t size() const {
        return workers.size();
    }

    // 获取当前队列中的任务数量
    size_t queueSize() const {
        std::unique_lock<std::mutex> lock(queueMutex);
        return tasks.size();
    }

    // 析构函数，停止所有工作线程
    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            stop = true;
        }
        
        condition.notify_all();  // 唤醒所有线程

        // 等待所有线程完成
        for (std::thread &worker : workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

private:
    std::vector<std::thread> workers;           // 工作线程集合
    std::queue<std::function<void()>> tasks;   // 任务队列

    mutable std::mutex queueMutex;              // 任务队列互斥锁
    std::condition_variable condition;          // 条件变量，用于线程同步
    std::atomic<bool> stop;                     // 线程池停止标志
};