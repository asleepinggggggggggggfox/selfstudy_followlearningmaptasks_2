#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <atomic>
#include <string>

class ThreadPool{
    public:
    ThreadPool(size_t numofthread);
    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    void workerThread();

    private:
    std::vector<std::thread> workers;   //工作线程
    std::queue<std::function<void()>> workqueue;    //任务队列

    mutable std::mutex queuemutex;
    std::condition_variable condition;
    std::atomic<bool> stop; //停止标志 用于析构
};








#endif