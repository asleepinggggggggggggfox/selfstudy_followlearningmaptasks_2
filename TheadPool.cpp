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
    for(int i = 0; i<numofthread;i++){
        workers.emplace_back([this]{
            this->workerThread();
        });
    }
}

ThreadPool::~ThreadPool(){
{
    std::unique_lock<std::mutex> lock();
    stop = true;
}

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
            if(stop && workqueue.empty()){
                return;
            }
            condition.wait(lock,[this]{
                return stop || !workqueue.empty();
            });
            task = std::move(workqueue.front());  
            workqueue.pop();
        }
        task();
    }
}