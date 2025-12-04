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
        task();
        free_counter--;
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