#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <atomic>
#include <string>
#include "ThreadPool.h"

ThreadPool::ThreadPool(size_t numofthread):stop(false){
    for(size_t i = 0; i<numofthread;i++){
        workers.emplace_back([this]{
            this->workerThread();
        });
    }
}

ThreadPool::~ThreadPool(){
{
    std::unique_lock<std::mutex> lock(queuemutex);
    stop = true;
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

            condition.wait(lock,[this]{
                return stop || !workqueue.empty();
            });
            if(stop && workqueue.empty()){
                return;
            }

            if(!workqueue.empty()){
            task = std::move(workqueue.front());  
            workqueue.pop();                
            }

        }
        task();
    }
}

size_t ThreadPool::threadsize() const{
    return workers.size();
}

size_t ThreadPool::workqueuesize() const{
    std::unique_lock<std::mutex> lock(queuemutex);
    return workqueue.size();
}