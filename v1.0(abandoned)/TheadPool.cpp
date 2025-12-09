#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <atomic>
#include <string>
#include <iostream>
#include "ThreadPool.h"


ThreadPool::ThreadPool(size_t numofthread):stopsign(false){
    for(size_t i = 0; i<numofthread;i++){
        workers.emplace_back([this]{
            this->workerThread();
        });
        

    }
}

ThreadPool::~ThreadPool(){
{
    std::unique_lock<std::mutex> lock(queuemutex);
    stopsign = true;
}
    condition.notify_all();  
    for(std::thread &worker:workers){
        if(worker.joinable()){
            worker.join();
        }
}

}
void ThreadPool::workerThread(){
    while(true){
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(queuemutex);
            free_counter++;
            condition.wait(lock,[this]{
                return stopsign || !workqueue.empty();
            });
            if(stopsign && workqueue.empty()){
                return;
            }

            if(!workqueue.empty()){
            task = std::move(workqueue.front());  
            workqueue.pop();                
            }

        }
        free_counter--;
        if(task){
            task();
        }
        else{
            return;
        }
        
        
    }
}

size_t ThreadPool::threadsize() const{
    return workers.size();
}

size_t ThreadPool::workqueuesize() const{
    std::unique_lock<std::mutex> lock(queuemutex);
    return workqueue.size();
}

size_t ThreadPool::freethreadsize() const{
    return free_counter;
}

void ThreadPool::stop(){
    stopsign = true;
    while (!workqueue.empty())
    {
        workqueue.pop();
    }
    condition.notify_all();
    for(std::thread &worker : workers){
        if(worker.joinable()){
            worker.join();
        }
    }
}

void ThreadPool::revise(size_t num) {
    if (num == 0) return;

    std::unique_lock<std::mutex> lock(queuemutex);
    size_t currentsize = workers.size();
    
    if (num == currentsize) return;
    
    std::cout << "Adjust the size of the thread pool: " << currentsize << " -> " << num << std::endl;
    
    if (num > currentsize) {
        // 扩容逻辑：在锁保护下创建新线程
        for (size_t i = currentsize; i < num; i++) {
            workers.emplace_back([this] {
                workerThread();
            });
        }
    } else {
        // 缩容逻辑：安全地减少线程
        stopsign = true; // 1. 设置停止标志，通知所有线程准备退出
    }
    lock.unlock(); // 释放锁，让线程可以执行退出逻辑

    // 2. 通知所有线程，让它们检查停止条件
    condition.notify_all();

    // 3. 等待需要被移除的线程结束
    size_t reduceCount = currentsize - num;
    for (size_t i = 0; i < reduceCount; i++) {
        if (workers.back().joinable()) {
            workers.back().join(); // 等待线程结束
        }
        workers.pop_back(); // 从向量中移除线程对象
    }

    lock.lock();
    stopsign = false;
}