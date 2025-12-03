#include "ThreadPool.h"
#include <iostream>
#include <chrono>
#include <vector>
#include <future>
#include <atomic>
#include <cassert>

// 简单的任务函数
int simpleTask(int a, int b) {
    return a + b;
}

// 模拟耗时任务
void timeConsumingTask(int milliseconds) {
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

// 测试基本功能
void testBasicFunctionality() {
    std::cout << "=== 测试基本功能 ===" << std::endl;
    
    ThreadPool pool(4);
    
    // 测试简单任务提交
    auto future1 = pool.enqueue(simpleTask, 10, 20);
    auto result1 = future1.get();
    std::cout << "简单任务结果: " << result1 << std::endl;
    assert(result1 == 30);
    
    // 测试lambda表达式
    auto future2 = pool.enqueue([]() { return std::string("Hello, ThreadPool!"); });
    auto result2 = future2.get();
    std::cout << "Lambda任务结果: " << result2 << std::endl;
    assert(result2 == "Hello, ThreadPool!");
    
    std::cout << "基本功能测试通过! ✓" << std::endl;
}

// 测试并发性能
void testConcurrentPerformance() {
    std::cout << "\n=== 测试并发性能 ===" << std::endl;
    
    ThreadPool pool(std::thread::hardware_concurrency());
    const int taskCount = 100;
    std::vector<std::future<int>> futures;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // 提交大量任务
    for (int i = 0; i < taskCount; ++i) {
        futures.emplace_back(pool.enqueue([i]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            return i * i;
        }));
    }
    
    // 收集结果
    for (int i = 0; i < taskCount; ++i) {
        assert(futures[i].get() == i * i);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "完成 " << taskCount << " 个任务耗时: " 
              << duration.count() << "ms" << std::endl;
    std::cout << "并发性能测试通过! ✓" << std::endl;
}

// 测试线程池状态查询
void testPoolStatus() {
    std::cout << "\n=== 测试线程池状态 ===" << std::endl;
    
    ThreadPool pool(2);
    
    std::cout << "线程数量: " << pool.threadsize() << std::endl;
    assert(pool.threadsize() == 2);
    
    std::cout << "初始队列大小: " << pool.workqueuesize() << std::endl;
    assert(pool.workqueuesize() == 0);
    
    // 提交一些任务
    std::vector<std::future<void>> futures;
    for (int i = 0; i < 5; ++i) {
        futures.emplace_back(pool.enqueue([i]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }));
    }
    
    // 短暂等待后检查队列大小
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    std::cout << "提交任务后队列大小: " << pool.workqueuesize() << std::endl;
    
    std::cout << "状态查询测试通过! ✓" << std::endl;
}

// 测试异常处理
void testExceptionHandling() {
    std::cout << "\n=== 测试异常处理 ===" << std::endl;
    
    ThreadPool pool(2);
    
    // 测试抛出异常的任务
    auto future = pool.enqueue([]() -> int {
        throw std::runtime_error("测试异常");
        return 42;
    });
    
    try {
        future.get();
        assert(false); // 不应该执行到这里
    } catch (const std::exception& e) {
        std::cout << "成功捕获异常: " << e.what() << std::endl;
    }
    
    std::cout << "异常处理测试通过! ✓" << std::endl;
}

// 测试析构函数行为
void testDestructorBehavior() {
    std::cout << "\n=== 测试析构函数行为 ===" << std::endl;
    
    {
        ThreadPool pool(2);
        
        // 提交一些任务但不等待完成
        for (int i = 0; i < 3; ++i) {
            pool.enqueue([i]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            });
        }
        
        // pool在作用域结束时析构，应该等待所有任务完成
    }
    
    std::cout << "析构函数测试完成! ✓" << std::endl;
}

// 性能基准测试
void benchmarkPerformance() {
    std::cout << "\n=== 性能基准测试 ===" << std::endl;
    
    const int heavyTaskCount = 1000;
    
    // 测试单线程执行
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < heavyTaskCount; ++i) {
        simpleTask(i, i);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto singleThreadTime = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // 测试线程池执行
    ThreadPool pool(std::thread::hardware_concurrency());
    start = std::chrono::high_resolution_clock::now();
    
    std::vector<std::future<int>> futures;
    for (int i = 0; i < heavyTaskCount; ++i) {
        futures.emplace_back(pool.enqueue(simpleTask, i, i));
    }
    
    for (auto& future : futures) {
        future.get();
    }
    
    end = std::chrono::high_resolution_clock::now();
    auto multiThreadTime = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "单线程执行时间: " << singleThreadTime.count() << "ms" << std::endl;
    std::cout << "线程池执行时间: " << multiThreadTime.count() << "ms" << std::endl;
    std::cout << "加速比: " << static_cast<double>(singleThreadTime.count()) / multiThreadTime.count() << "x" << std::endl;
}

int main() {
    std::cout << "开始测试线程池..." << std::endl;
    
    try {
        testBasicFunctionality();
        testConcurrentPerformance();
        testPoolStatus();
        testExceptionHandling();
        testDestructorBehavior();
        benchmarkPerformance();
        
        std::cout << "\n🎉 所有测试通过! 线程池功能正常。" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "❌ 测试失败: " << e.what() << std::endl;
        return 1;
    }
}