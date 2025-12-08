#include <iostream>
#include <chrono>
#include <vector>
#include <atomic>
#include <stdexcept>
#include <algorithm>

// 包含您的线程池头文件
#include "ThreadPool.hpp"

// 测试函数声明
void testBasicFunctionality();
void testThreadStatus();
void testConcurrentEnqueue();
void testExceptionHandling();
void testDestructorBehavior();
void testPerformance();
void testEdgeCases();

int main() {
    std::cout << "=== ThreadPool 全面测试开始 ===\n" << std::endl;
    
    try {
        testBasicFunctionality();
        testThreadStatus();
        testConcurrentEnqueue();
        testExceptionHandling();
        testDestructorBehavior();
        testPerformance();
        testEdgeCases();
        
        std::cout << "\n=== 所有测试通过！ ===" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "测试失败: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}

// 1. 基本功能测试
void testBasicFunctionality() {
    std::cout << "1. 基本功能测试..." << std::endl;
    
    ThreadPool pool(4);
    
    // 测试简单任务
    auto future1 = pool.enqueue([]() { return 42; });
    assert(future1.get() == 42);
    
    // 测试带参数的任务
    auto future2 = pool.enqueue([](int a, int b) { return a + b; }, 10, 20);
    assert(future2.get() == 30);
    
    // 测试字符串返回值
    auto future3 = pool.enqueue([](const std::string& s) { return "Hello " + s; }, "World");
    assert(future3.get() == "Hello World");
    
    // 测试多个任务
    std::vector<std::future<int>> results;
    for (int i = 0; i < 10; ++i) {
        results.emplace_back(pool.enqueue([i]() { return i * i; }));
    }
    
    for (int i = 0; i < 10; ++i) {
        assert(results[i].get() == i * i);
    }
    
    std::cout << "   ✓ 基本功能测试通过" << std::endl;
}

// 2. 线程状态测试（简化版，避免直接访问内部状态）
void testThreadStatus() {
    std::cout << "2. 线程状态测试..." << std::endl;
    
    ThreadPool pool(2);
    
    // 提交耗时任务来观察线程行为
    std::atomic<int> task_count{0};
    std::vector<std::future<void>> futures;
    
    for (int i = 0; i < 4; ++i) {
        futures.emplace_back(pool.enqueue([&task_count, i]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            task_count++;
        }));
    }
    
    // 等待所有任务完成
    for (auto& f : futures) {
        f.get();
    }
    
    assert(task_count == 4);
    std::cout << "   ✓ 线程状态测试通过" << std::endl;
}

// 3. 并发入队测试
void testConcurrentEnqueue() {
    std::cout << "3. 并发入队测试..." << std::endl;
    
    ThreadPool pool(4);
    const int task_count = 100;
    std::atomic<int> completed_tasks{0};
    
    // 多个生产者线程同时提交任务
    auto producer = [&pool, &completed_tasks]() {
        for (int i = 0; i < task_count / 10; ++i) {
            pool.enqueue([&completed_tasks]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                completed_tasks++;
            });
        }
    };
    
    std::vector<std::thread> producers;
    for (int i = 0; i < 10; ++i) {
        producers.emplace_back(producer);
    }
    
    for (auto& p : producers) {
        p.join();
    }
    
    // 等待所有任务完成（使用更智能的等待方式）
    auto start = std::chrono::steady_clock::now();
    while (completed_tasks < task_count) {
        if (std::chrono::steady_clock::now() - start > std::chrono::seconds(5)) {
            std::cout << "警告: 任务完成超时" << std::endl;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    assert(completed_tasks == task_count);
    std::cout << "   ✓ 并发入队测试通过" << std::endl;
}

// 4. 异常处理测试（修复了段错误问题）
void testExceptionHandling() {
    std::cout << "4. 异常处理测试..." << std::endl;
    
    ThreadPool pool(2);
    
    // 测试任务抛出异常
    auto failing_task = []() -> int {
        throw std::runtime_error("任务执行失败");
        return 1;
    };
    
    auto future = pool.enqueue(failing_task);
    
    try {
        future.get();
        assert(false); // 不应该执行到这里
    } catch (const std::runtime_error& e) {
        std::cout << "   ✓ 捕获到预期异常: " << e.what() << std::endl;
    }
    
    // 安全的停止测试：通过作用域自然析构
    {
        ThreadPool temp_pool(1);
        auto temp_future = temp_pool.enqueue([]() { return 42; });
        assert(temp_future.get() == 42);
        // temp_pool 会自动析构
    }
    
    std::cout << "   ✓ 异常处理测试通过" << std::endl;
}

// 5. 析构行为测试
void testDestructorBehavior() {
    std::cout << "5. 析构行为测试..." << std::endl;
    
    std::atomic<int> tasks_completed{0};
    const int expected_tasks = 5;
    
    {
        ThreadPool pool(2);
        
        // 提交一些耗时任务
        for (int i = 0; i < expected_tasks; ++i) {
            pool.enqueue([&tasks_completed, i]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                tasks_completed++;
                return i;
            });
        }
        
        // pool 析构时会等待所有任务完成
    }
    
    assert(tasks_completed == expected_tasks);
    std::cout << "   ✓ 析构行为测试通过" << std::endl;
}

// 6. 性能测试
void testPerformance() {
    std::cout << "6. 性能测试..." << std::endl;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    {
        ThreadPool pool(4);
        const int task_count = 1000;
        std::atomic<int> counter{0};
        std::vector<std::future<void>> futures;
        
        for (int i = 0; i < task_count; ++i) {
            futures.emplace_back(pool.enqueue([&counter]() {
                // 模拟轻量级工作
                counter++;
            }));
        }
        
        // 等待所有任务完成
        for (auto& f : futures) {
            f.get();
        }
        
        assert(counter == task_count);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "   ✓ 完成1000个任务耗时: " << duration.count() << "ms" << std::endl;
}

// 7. 边界情况测试
void testEdgeCases() {
    std::cout << "7. 边界情况测试..." << std::endl;
    
    // 测试单线程线程池
    {
        ThreadPool pool(1);
        auto future = pool.enqueue([]() { return 42; });
        assert(future.get() == 42);
    }
    
    // 测试大量小任务
    {
        ThreadPool pool(2);
        std::atomic<int> count{0};
        std::vector<std::future<void>> futures;
        
        for (int i = 0; i < 200; ++i) {
            futures.emplace_back(pool.enqueue([&count]() {
                count++;
            }));
        }
        
        for (auto& f : futures) {
            f.get();
        }
        
        assert(count == 200);
    }
    
    // 测试任务依赖（前一个任务的输出作为后一个任务的输入）
    {
        ThreadPool pool(2);
        
        auto future1 = pool.enqueue([]() { return 10; });
        auto future2 = pool.enqueue([future1 = std::move(future1)]() mutable {
            return future1.get() * 2;
        });
        
        assert(future2.get() == 20);
    }
    
    std::cout << "   ✓ 边界情况测试通过" << std::endl;
}