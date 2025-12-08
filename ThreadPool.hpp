#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <atomic>
#include <string>



enum class ThreadStatus{
    idle,
    busy,
    stopped
};

struct ThreadText{
    std::thread thread;
    std::atomic<ThreadStatus> status{ThreadStatus::idle};
    std::thread::id threadId;
    std::atomic<bool> stop;
};

class ThreadPool {
    public:
        ThreadPool(size_t numofthread){
            threadTexts.resize(numofthread);
            for(size_t i=0;i<numofthread;++i){
                threadTexts[i].stop = false;
                threadTexts[i].thread = std::thread(&ThreadPool::workerThread,this);
                threadTexts[i].threadId = threadTexts[i].thread.get_id();
            }
        }

    private:
        std::vector<ThreadText> threadTexts;            //线程信息
        std::queue<std::function<void()>> workqueue;    //任务队列

        std::mutex queuemutex;                          //任务队列互斥锁
        std::condition_variable condition;              //条件变量
        std::atomic<bool> stopall{false};              //停止标志



};