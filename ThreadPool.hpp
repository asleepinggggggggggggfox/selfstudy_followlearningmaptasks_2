#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <atomic>
#include <string>
#include <cassert>


enum class ThreadStatus{
    idle,
    busy,
    stopped
};

struct ThreadText {
    std::thread thread;
    std::atomic<ThreadStatus> status{ThreadStatus::idle};
    std::thread::id threadId{};
    std::atomic<bool> stop{false};

    ThreadText() = default;
    ThreadText(const ThreadText&) = delete;
    ThreadText& operator=(const ThreadText&) = delete;
    
    // 移动构造函数
    ThreadText(ThreadText&& other) noexcept 
        : thread(std::move(other.thread))
        , status(other.status.load())
        , threadId(other.threadId)
        , stop(other.stop.load()) {
    }
    
    // 移动赋值运算符
    ThreadText& operator=(ThreadText&& other) noexcept {
        if (this != &other) {
            thread = std::move(other.thread);
            status.store(other.status.load());
            threadId = other.threadId;
            stop.store(other.stop.load());
        }
        return *this;
    }
};

class ThreadPool {
public:
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

    ThreadPool(size_t numofthread) : stopall{false} {
        if (numofthread == 0) {
            throw std::invalid_argument("nums of threads must >0");
        }
        
        threadTexts.reserve(numofthread);
        for(size_t i = 0; i < numofthread; ++i) {
            threadTexts.emplace_back();
            threadTexts[i].thread = std::thread(&ThreadPool::workerThread, this);
            threadTexts[i].threadId = threadTexts[i].thread.get_id();
        }
    }
    void workerThread(){
        std::thread::id this_id = std::this_thread::get_id();
        int index = -1;
        for(size_t i = 0; i < threadTexts.size(); ++i){
            if(threadTexts[i].threadId == this_id){
                index = i;
                break;
            }
        }
        if(index == -1) {
        std::cerr << "错误：工作线程未在线程列表中找到！" << std::endl;
        return; // 安全退出
    }
        while(true){
        std::function<void()> task;
        threadTexts[index].status.store(ThreadStatus::idle);
        {
        std::unique_lock<std::mutex> lock(queuemutex);
        condition.wait(lock,[this]{
            return stopall || !workqueue.empty();
            });
        if(stopall && workqueue.empty()){
            threadTexts[index].status.store(ThreadStatus::stopped);
            return;
            }
            if(!workqueue.empty()){
            task = std::move(workqueue.front());  
            workqueue.pop();                
            }

        }
        if(task){
            threadTexts[index].status.store(ThreadStatus::busy);
            task();
        }   
    }
    }

    template<class F, class... Args>
    auto enqueue(F&&f,Args&& ...args)->std::future<typename std::result_of<F(Args...)>::type>{ //声明和定义要放在一起
        using return_type = typename std::result_of<F(Args...)>::type;
        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        std::future<return_type> result = task->get_future();
        {
            std::unique_lock<std::mutex> lock(queuemutex);

            if(stopall){
                throw std::runtime_error("enqueue on stopped ThreadPool");
            }
            workqueue.emplace([task](){(*task)();}); //存入lamda表达式 工作线程进行调用
        }
        condition.notify_one();
        return result;
    }


    ~ThreadPool(){
        {
            std::unique_lock<std::mutex> lock(queuemutex);
            stopall = true;
        }
        condition.notify_all();
        for(ThreadText &workerText:threadTexts){
            if(workerText.thread.joinable()){
                workerText.thread.join();
            }
        }
    }






    size_t workqueuesize() const{
    std::unique_lock<std::mutex> lock(queuemutex);
    return workqueue.size();
    }


private:
    std::vector<ThreadText> threadTexts;            //线程信息
    std::queue<std::function<void()>> workqueue;    //任务队列

    mutable std::mutex queuemutex;                          //任务队列互斥锁
    std::condition_variable condition;              //条件变量
    std::atomic<bool> stopall;              //停止标志
};