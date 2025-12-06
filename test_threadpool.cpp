#include "ThreadPool.h"
#include <iostream>
#include <chrono>
#include <vector>
#include <future>
#include <atomic>
#include <cassert>

using namespace std;

// 测试任务：计算累加和
int accumulateTask(int start, int end) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 模拟耗时操作
    int sum = 0;
    for (int i = start; i <= end; ++i) {
        sum += i;
    }
    return sum;
}
std::mutex coutMutex; // 用于保护std::cout
// 测试任务：无返回值，仅打印信息
void printTask(int id, const std::string& message) {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    std::unique_lock<std::mutex> lock(coutMutex);
    std::cout << "Task " << id << ": " << message 
              << " (Thread: " << std::this_thread::get_id() << ")" << std::endl;
}



// 基本功能测试
void testBasicFunctionality() {
    std::cout << "=== 基本功能测试 ===" << std::endl;
    
    ThreadPool pool(4);
    std::vector<std::future<int>> results;
    
    // 提交多个任务
    for (int i = 0; i < 8; ++i) {
        results.push_back(pool.enqueue(accumulateTask, i * 100, (i + 1) * 100));
    }
    
    // 获取结果
    for (size_t i = 0; i < results.size(); ++i) {
        int result = results[i].get();
        std::cout << "Task " << i << " result: " << result << std::endl;
    }
    
    std::cout << "当前空闲线程数: " << pool.freethreadsize() << std::endl;
    std::cout << "基本功能测试通过!\n" << std::endl;
}

// 线程池调整测试
void testResizeFunctionality() {
    std::cout << "=== 线程池调整测试 ===" << std::endl;
    
    ThreadPool pool(2);
    std::cout << "初始线程数: " << pool.threadsize() << std::endl;
    
    // 测试扩大线程池
    pool.revise(6);
    std::cout << "扩大后线程数: " << pool.threadsize() << std::endl;
    
    // 提交一些任务测试新线程
    std::vector<std::future<void>> tasks;
    for (int i = 0; i < 10; ++i) {
        tasks.push_back(pool.enqueue(printTask, i, "Resize test"));
    }
    
    // 等待所有任务完成
    for (auto& task : tasks) {
        task.wait();
    }
    
    // 测试缩小线程池
    pool.revise(3);
    std::cout << "缩小后线程数: " << pool.threadsize() << std::endl;
    
    std::cout << "线程池调整测试通过!\n" << std::endl;
}

// 状态查询测试
void testStatusQueries() {
    std::cout << "=== 状态查询测试 ===" << std::endl;
    
    ThreadPool pool(3);
    
    std::cout << "初始状态 - 线程数: " << pool.threadsize() 
              << ", 空闲线程: " << pool.freethreadsize() 
              << ", 任务队列: " << pool.workqueuesize() << std::endl;
    
    // 提交超过线程数的任务
    std::vector<std::future<int>> tasks;
    for (int i = 0; i < 10; ++i) {
        tasks.push_back(pool.enqueue([](int x) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            return x * x;
        }, i));
    }
    
    // 立即查询状态（任务应堆积在队列中）
    std::cout << "提交任务后 - 线程数: " << pool.threadsize() 
              << ", 空闲线程: " << pool.freethreadsize() 
              << ", 任务队列: " << pool.workqueuesize() << std::endl;
    
    // 等待所有任务完成
    for (auto& task : tasks) {
        task.wait();
    }
    
    std::cout << "任务完成后 - 线程数: " << pool.threadsize() 
              << ", 空闲线程: " << pool.freethreadsize() 
              << ", 任务队列: " << pool.workqueuesize() << std::endl;
    
    std::cout << "状态查询测试通过!\n" << std::endl;
}

// 异常情况测试
void testExceptionHandling() {
    std::cout << "=== 异常情况测试 ===" << std::endl;
    
    ThreadPool pool(2);
    
    // 测试异常传播
    auto exceptionTask = pool.enqueue([]() {
        throw std::runtime_error("测试异常");
        return 42;
    });
    
    try {
        exceptionTask.get();
        std::cout << "异常未正确传播!" << std::endl;
    } catch (const std::exception& e) {
        std::cout << "正确捕获异常: " << e.what() << std::endl;
    }
    
    std::cout << "异常情况测试通过!\n" << std::endl;
}

// 性能测试
void testPerformance() {
    std::cout << "=== 性能测试 ===" << std::endl;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    ThreadPool pool(4);
    std::vector<std::future<long long>> results;
    
    // 提交计算密集型任务
    for (int i = 0; i < 20; ++i) {
        results.push_back(pool.enqueue([](int n) -> long long {
            long long sum = 0;
            for (int i = 1; i <= n; ++i) {
                sum += i;
            }
            return sum;
        }, 1000000));
    }
    
    // 收集结果
    long long total = 0;
    for (auto& result : results) {
        total += result.get();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "总计算结果: " << total << std::endl;
    std::cout << "执行时间: " << duration.count() << "ms" << std::endl;
    std::cout << "性能测试完成!\n" << std::endl;
}

// 停止功能测试
void testStopFunctionality() {
    std::cout << "=== 停止功能测试 ===" << std::endl;
    
    {
        ThreadPool pool(2);
        
        // 提交一些任务
        for (int i = 0; i < 5; ++i) {
            pool.enqueue(printTask, i, "Before stop");
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "准备停止线程池..." << std::endl;
        
        // 析构函数会自动调用stop
    } // pool离开作用域，调用析构函数
    
    std::cout << "线程池已停止!\n" << std::endl;
}

int main() {
    std::cout << "开始测试线程池..." << std::endl;
    
    try {
        testBasicFunctionality();
        testResizeFunctionality();
        testStatusQueries();
        testExceptionHandling();
        testPerformance();
        testStopFunctionality();
        
        std::cout << "=== 所有测试通过! ===" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "测试失败: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}