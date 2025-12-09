// ThreadPool_Fixed.hpp
#pragma once

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <atomic>
#include <memory>
#include <iostream>
#include <chrono>
#include <algorithm>

class ThreadPool {
public:
    explicit ThreadPool(size_t min_threads = std::thread::hardware_concurrency(),
                       size_t max_threads = std::thread::hardware_concurrency() * 2)
        : shutdown_(false), min_threads_(min_threads), max_threads_(max_threads) 
    {
        // 先创建所有线程，但不立即启动工作循环
        for (size_t i = 0; i < min_threads_; ++i) {
            workers_.emplace_back([this]() { 
                // 短暂的延迟，确保主线程完成初始化
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                worker_loop(); 
            });
        }
        std::cout << "ThreadPool initialized with " << min_threads_ << " threads, max: " << max_threads_ << std::endl;
        
        // 等待所有工作线程真正启动
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    template<class F, class... Args>
    auto submit(F&& f, Args&&... args) 
        -> std::future<typename std::result_of<F(Args...)>::type> {
        
        using return_type = typename std::result_of<F(Args...)>::type;
        
        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        
        std::future<return_type> result = task->get_future();

        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            
            if(shutdown_) {
                throw std::runtime_error("submit called on stopped ThreadPool");
            }

            tasks_.emplace([task](){ (*task)(); });

            // 更保守的扩容策略
            if (tasks_.size() > 2 && get_idle_count_safe() == 0 && workers_.size() < max_threads_) {
                workers_.emplace_back([this]() { 
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    worker_loop(); 
                });
                std::cout << "Dynamic expansion: Thread created. Total: " << workers_.size() << std::endl;
            }
        }

        condition_.notify_one();
        return result;
    }

    size_t get_thread_count() const {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        return workers_.size();
    }

    size_t get_queue_size() const {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        return tasks_.size();
    }

    // 安全的空闲线程计数（无锁版本）
    size_t get_idle_count_safe() const {
        return idle_count_.load();
    }

    void wait_all() {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        completion_condition_.wait(lock, [this]() {
            return tasks_.empty() && (idle_count_ == workers_.size());
        });
    }

    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            shutdown_ = true;
        }
        condition_.notify_all();
        completion_condition_.notify_all();
        
        for (auto &worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
        std::cout << "ThreadPool destroyed successfully." << std::endl;
    }

private:
    std::atomic<bool> shutdown_{false};
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    mutable std::mutex queue_mutex_;
    std::condition_variable condition_;
    std::condition_variable completion_condition_;
    std::atomic<size_t> idle_count_{0};
    size_t min_threads_;
    size_t max_threads_;

    void worker_loop() {
        auto my_id = std::this_thread::get_id();
        
        while (true) {
            std::function<void()> task;
            bool should_exit = false;

            {
                std::unique_lock<std::mutex> lock(queue_mutex_);
                idle_count_++;

                // 关键修复：简化等待条件，避免复杂逻辑
                condition_.wait(lock, [this]() { 
                    return shutdown_ || !tasks_.empty(); 
                });

                if (shutdown_ && tasks_.empty()) {
                    idle_count_--;
                    should_exit = true;
                } else if (!tasks_.empty()) {
                    task = std::move(tasks_.front());
                    tasks_.pop();
                    idle_count_--;
                }
                
                // 如果should_exit为true，直接退出循环
                if (should_exit) {
                    break;
                }
            }

            // 在锁外执行任务，避免阻塞其他线程
            if (task) {
                try {
                    task();
                } catch (const std::exception& e) {
                    std::cerr << "Task execution error in thread " << my_id << ": " << e.what() << std::endl;
                } catch (...) {
                    std::cerr << "Unknown task execution error in thread " << my_id << std::endl;
                }
            }

            
            check_and_scale_down_simple();
        }
    }

    void check_and_scale_down_simple() { //动态缩容
        
    }
};