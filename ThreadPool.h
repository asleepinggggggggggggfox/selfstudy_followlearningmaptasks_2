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

    template<class F, class... Args>
    auto enqueue(F&&f,Args&& ...args)->std::future<typename std::result_of<F(Args...)>::type>{ //声明和定义要放在一起
        using return_type = typename std::result_of<F(Args...)>::type;
        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        std::future<return_type> result = task->get_future();
        {
            std::unique_lock<std::mutex> lock(queuemutex);

            if(stopsign){
                throw std::runtime_error("enqueue on stopped ThreadPool");
            }
            workqueue.emplace([task](){(*task)();}); //存入lamda表达式 工作线程进行调用


        }
        condition.notify_one();
        return result;
    }


    void revise(size_t num);

    void stop();


    size_t threadsize() const;
    size_t workqueuesize() const;
    size_t freethreadsize() const;

    void appendtobusyworkers(std::thread* th);
    void removebusyworkers(std::thread* th);

    private:
    std::vector<std::thread> workers;   //工作线程
    std::queue<std::function<void()>> workqueue;    //任务队列
    std::atomic<size_t> free_counter{0};      //线程安全的原子计数器
    std::vector<std::thread*> busyworkers;
    std::vector<std::thread*> freeworkers;



    mutable std::mutex queuemutex;
    mutable std::mutex listmutex;

    std::condition_variable condition;
    std::atomic<bool> stopsign; //停止标志 用于析构
};








#endif