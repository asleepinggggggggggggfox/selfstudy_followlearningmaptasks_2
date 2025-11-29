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

class DynamicThreadPool : public thread_oprea{
public:
    // 构造函数：设置线程数量的上下限
    DynamicThreadPool(size_t minThreads, size_t maxThreads);
    
    // 析构函数：优雅关闭
    ~DynamicThreadPool();

    // 提交任务：使用可变参数模板和返回future
    template<class F, class... Args>
    auto submit(F&& f, Args&&... args) -> std::future<decltype(f(args...))>;

    // 动态调整线程池大小（可选手动干预）
    void resize(size_t newSize);
    


    // 禁止拷贝
    DynamicThreadPool(const DynamicThreadPool&) = delete;
    DynamicThreadPool& operator=(const DynamicThreadPool&) = delete;

private:
    // 工作线程的主函数
    void workerRoutine();

    // 内部成员变量
    std::vector<std::thread> workers_;          // 工作线程集合
    std::queue<std::function<void()>> tasks_;   // 任务队列

    std::mutex queueMutex_;                     // 保护任务队列的互斥锁
    std::condition_variable condition_;         // 用于任务通知的条件变量

    std::atomic<bool> stop_{false};            // 停止标志
    std::atomic<size_t> idleThreads_{0};       // 空闲线程计数

    size_t coreThreads_;                  //核心线程数
    size_t maxThreads_;                  //最大线程数
    size_t getTaskQueueSize() const;    //获取工作队列
    
    // 获取线程状态信息
    size_t getBusyThreadCount() const;
    size_t getIdleThreadCount() const;



};
class thread_oprea{
// 要有能创造并自动命名（thread_1...n） 销毁线程的接口 能读取线程名字 状态 
private:

public:
    thread_oprea();
    ~thread_oprea();
    std::string CreatThread();
    int DestoryThread();



};


#endif