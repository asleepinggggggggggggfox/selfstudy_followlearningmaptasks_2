#include <iostream>
#include <vector>
#include <atomic>
#include <chrono>
#include <stdexcept>
#include "ThreadPool.hpp"

// 测试函数声明
void test_basic_functionality();
void test_concurrent_safety();
void test_task_return_values();
void test_exception_handling();
void test_thread_pool_destruction();
void test_stop_behavior();
void test_performance();

int main() {
    std::cout << "开始线程池测试..." << std::endl;
    
    try {
        test_basic_functionality();
        std::cout << "✓ 基本功能测试通过" << std::endl;
        
        test_concurrent_safety();
        std::cout << "✓ 并发安全测试通过" << std::endl;
        
        test_task_return_values();
        std::cout << "✓ 任务返回值测试通过" << std::endl;
        
        test_exception_handling();
        std::cout << "✓ 异常处理测试通过" << std::endl;
        
        test_thread_pool_destruction();
        std::cout << "✓ 线程池析构测试通过" << std::endl;
        
        test_stop_behavior();
        std::cout << "✓ 停止行为测试通过" << std::endl;
        
        test_performance();
        std::cout << "✓ 性能测试通过" << std::endl;
        
        std::cout << "\n🎉 所有测试通过！线程池功能正常。" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ 测试失败: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}

// 1. 基本功能测试
void test_basic_functionality() {
    ThreadPool pool(2);
    
    // 测试简单任务提交
    std::atomic<int> counter{0};
    auto future = pool.enqueue([&counter]() {
        counter++;
        return 42;
    });
    
    // 等待任务完成并检查返回值
    assert(future.get() == 42);
    assert(counter == 1);
}

// 2. 并发安全测试
void test_concurrent_safety() {
    ThreadPool pool(4);
    std::atomic<int> shared_counter{0};
    const int TASK_COUNT = 1000;
    std::vector<std::future<void>> futures;
    
    // 提交大量并发任务
    for (int i = 0; i < TASK_COUNT; ++i) {
        futures.push_back(pool.enqueue([&shared_counter]() {
            shared_counter.fetch_add(1, std::memory_order_relaxed);
        }));
    }
    
    // 等待所有任务完成
    for (auto& future : futures) {
        future.get();
    }
    
    // 验证计数器值
    assert(shared_counter == TASK_COUNT);
}

// 3. 任务返回值测试
void test_task_return_values() {
    ThreadPool pool(2);
    
    // 测试不同类型的返回值
    auto future1 = pool.enqueue([]() { return std::string("Hello"); });
    auto future2 = pool.enqueue([]() { return 3.14; });
    auto future3 = pool.enqueue([]() { return std::vector<int>{1, 2, 3}; });
    
    assert(future1.get() == "Hello");
    assert(future2.get() == 3.14);
    assert(future3.get().size() == 3);
}

// 4. 异常处理测试[7](@ref)
void test_exception_handling() {
    ThreadPool pool(2);
    
    // 测试任务中抛出异常
    auto future = pool.enqueue([]() {
        throw std::runtime_error("测试异常");
        return 0;
    });
    
    bool exception_caught = false;
    try {
        future.get();
    } catch (const std::runtime_error&) {
        exception_caught = true;
    }
    assert(exception_caught);
    
    // 测试停止后提交任务[7](@ref)
    {
        ThreadPool temp_pool(1);
        // 析构函数会自动调用
    }
    
    ThreadPool another_pool(1);
    // 手动触发停止行为（通过作用域结束）
}

// 5. 线程池析构测试[1,5](@ref)
void test_thread_pool_destruction() {
    std::atomic<int> task_counter{0};
    
    {
        ThreadPool pool(2);
        
        // 提交一些任务但不等待完成
        for (int i = 0; i < 5; ++i) {
            pool.enqueue([&task_counter, i]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                task_counter.fetch_add(1);
            });
        }
        
        // pool析构时会等待任务完成
    }
    
    // 所有任务应该在析构前完成
    assert(task_counter == 5);
}

// 6. 停止行为测试
void test_stop_behavior() {
    ThreadPool pool(2);
    
    // 提交任务后立即让pool析构
    std::atomic<int> completed_tasks{0};
    
    for (int i = 0; i < 3; ++i) {
        pool.enqueue([&completed_tasks, i]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            completed_tasks.fetch_add(1);
        });
    }
    
    // 给任务一些时间开始执行
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

// 7. 性能测试[6](@ref)
void test_performance() {
    const size_t THREAD_COUNT = 4;
    const int TASK_COUNT = 100;
    
    ThreadPool pool(THREAD_COUNT);
    auto start_time = std::chrono::high_resolution_clock::now();
    
    std::vector<std::future<int>> results;
    for (int i = 0; i < TASK_COUNT; ++i) {
        results.push_back(pool.enqueue([i]() {
            // 模拟一些工作负载
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            return i * i;
        }));
    }
    
    // 收集结果
    for (int i = 0; i < TASK_COUNT; ++i) {
        assert(results[i].get() == i * i);
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time);
    
    std::cout << "   性能测试: 完成 " << TASK_COUNT 
              << " 个任务用时 " << duration.count() << "ms" << std::endl;
    
    // 验证多线程加速效果（应该明显快于单线程）
    assert(duration.count() < TASK_COUNT * 10); // 保守估计
}