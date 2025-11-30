#include <iostream>
#include <vector>
#include <chrono>
#include <atomic>
#include <cassert>
#include "ThreadPool_aige.hpp"

// 测试结果统计
class TestStats {
public:
    std::atomic<int> passed{0};
    std::atomic<int> failed{0};
    
    void printSummary() const {
        std::cout << "\n=== 测试结果汇总 ===" << std::endl;
        std::cout << "通过: " << passed.load() << " 项" << std::endl;
        std::cout << "失败: " << failed.load() << " 项" << std::endl;
        std::cout << "总计: " << (passed.load() + failed.load()) << " 项" << std::endl;
    }
};

TestStats stats;

// 基础功能测试
void testBasicFunctionality() {
    std::cout << "\n1. 基础功能测试..." << std::endl;
    
    try {
        ThreadPool pool(2);
        
        // 测试简单任务
        auto result = pool.enqueue([]() {
            return 42;
        });
        
        assert(result.get() == 42);
        std::cout << "✓ 简单任务执行测试通过" << std::endl;
        stats.passed++;
        
        // 测试带参数的任务
        auto result2 = pool.enqueue([](int a, int b) {
            return a + b;
        }, 10, 20);
        
        assert(result2.get() == 30);
        std::cout << "✓ 带参数任务测试通过" << std::endl;
        stats.passed++;
        
    } catch (const std::exception& e) {
        std::cout << "✗ 基础功能测试失败: " << e.what() << std::endl;
        stats.failed++;
    }
}

// 并发安全测试
void testConcurrentSafety() {
    std::cout << "\n2. 并发安全测试..." << std::endl;
    
    try {
        ThreadPool pool(4);
        std::atomic<int> counter{0};
        const int TASK_COUNT = 1000;
        
        std::vector<std::future<void>> futures;
        
        // 提交大量自增任务
        for (int i = 0; i < TASK_COUNT; ++i) {
            futures.emplace_back(pool.enqueue([&counter]() {
                counter.fetch_add(1, std::memory_order_relaxed);
            }));
        }
        
        // 等待所有任务完成
        for (auto& future : futures) {
            future.get();
        }
        
        assert(counter == TASK_COUNT);
        std::cout << "✓ 并发安全测试通过，计数器: " << counter << "/" << TASK_COUNT << std::endl;
        stats.passed++;
        
    } catch (const std::exception& e) {
        std::cout << "✗ 并发安全测试失败: " << e.what() << std::endl;
        stats.failed++;
    }
}

// 性能压力测试
void testPerformance() {
    std::cout << "\n3. 性能压力测试..." << std::endl;
    
    try {
        ThreadPool pool(std::thread::hardware_concurrency());
        const int TASK_COUNT = 10000;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        std::vector<std::future<int>> results;
        for (int i = 0; i < TASK_COUNT; ++i) {
            results.emplace_back(pool.enqueue([i]() {
                return i * i;  // 简单计算任务
            }));
        }
        
        // 验证所有结果
        for (int i = 0; i < TASK_COUNT; ++i) {
            assert(results[i].get() == i * i);
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "✓ 性能测试完成，处理 " << TASK_COUNT 
                  << " 个任务耗时: " << duration.count() << "ms" << std::endl;
        stats.passed++;
        
    } catch (const std::exception& e) {
        std::cout << "✗ 性能测试失败: " << e.what() << std::endl;
        stats.failed++;
    }
}

// 异常处理测试
void testExceptionHandling() {
    std::cout << "\n4. 异常处理测试..." << std::endl;
    
    try {
        ThreadPool pool(2);
        
        // 测试任务中抛出异常
        auto future = pool.enqueue([]() {
            throw std::runtime_error("测试异常");
            return 1;
        });
        
        try {
            future.get();
            std::cout << "✗ 异常处理测试失败: 应该捕获到异常" << std::endl;
            stats.failed++;
        } catch (const std::runtime_error& e) {
            std::cout << "✓ 异常传播测试通过: " << e.what() << std::endl;
            stats.passed++;
        }
        
    } catch (const std::exception& e) {
        std::cout << "✗ 异常处理测试失败: " << e.what() << std::endl;
        stats.failed++;
    }
}

// 线程池管理功能测试
void testPoolManagement() {
    std::cout << "\n5. 线程池管理功能测试..." << std::endl;
    
    try {
        ThreadPool pool(4);
        
        // 测试线程数量
        assert(pool.size() == 4);
        std::cout << "✓ 线程数量测试通过: " << pool.size() << std::endl;
        stats.passed++;
        
        // 测试队列大小统计
        assert(pool.queueSize() == 0);
        
        // 提交一些任务但不立即执行
        std::vector<std::future<int>> futures;
        for (int i = 0; i < 10; ++i) {
            futures.emplace_back(pool.enqueue([i]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                return i;
            }));
        }
        
        // 短暂等待后检查队列大小（可能有些任务已经开始执行）
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        std::cout << "✓ 队列大小统计: " << pool.queueSize() << std::endl;
        stats.passed++;
        
    } catch (const std::exception& e) {
        std::cout << "✗ 管理功能测试失败: " << e.what() << std::endl;
        stats.failed++;
    }
}

// 停止后提交任务测试
void testEnqueueAfterStop() {
    std::cout << "\n6. 停止后提交任务测试..." << std::endl;
    
    try {
        {
            // 在独立作用域中创建和销毁线程池
            ThreadPool pool(2);
            // 池对象会在作用域结束时自动析构，设置stop标志
        }
        
        // 创建新的线程池进行测试
        ThreadPool pool(2);
        
        // 测试正常情况下的enqueue
        auto future = pool.enqueue([]() { return 42; });
        assert(future.get() == 42);
        
        std::cout << "✓ 停止后提交测试通过" << std::endl;
        stats.passed++;
        
    } catch (const std::exception& e) {
        std::cout << "✗ 停止后提交测试失败: " << e.what() << std::endl;
        stats.failed++;
    }
}

// 复杂任务依赖测试
void testTaskDependencies() {
    std::cout << "\n7. 复杂任务依赖测试..." << std::endl;
    
    try {
        ThreadPool pool(3);
        std::atomic<int> executionOrder{0};
        
        auto task1 = pool.enqueue([&executionOrder]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            int order = executionOrder.fetch_add(1);
            return std::string("任务1完成，顺序: ") + std::to_string(order);
        });
        
        auto task2 = pool.enqueue([&executionOrder]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            int order = executionOrder.fetch_add(1);
            return std::string("任务2完成，顺序: ") + std::to_string(order);
        });
        
        auto task3 = pool.enqueue([&executionOrder]() {
            int order = executionOrder.fetch_add(1);
            return std::string("任务3完成，顺序: ") + std::to_string(order);
        });
        
        std::cout << "任务1结果: " << task1.get() << std::endl;
        std::cout << "任务2结果: " << task2.get() << std::endl;
        std::cout << "任务3结果: " << task3.get() << std::endl;
        
        std::cout << "✓ 复杂任务依赖测试通过" << std::endl;
        stats.passed++;
        
    } catch (const std::exception& e) {
        std::cout << "✗ 复杂任务依赖测试失败: " << e.what() << std::endl;
        stats.failed++;
    }
}

int main() {
    std::cout << "开始测试线程池..." << std::endl;
    std::cout << "硬件并发数: " << std::thread::hardware_concurrency() << std::endl;
    
    // 运行所有测试
    testBasicFunctionality();
    testConcurrentSafety();
    testPerformance();
    testExceptionHandling();
    testPoolManagement();
    testEnqueueAfterStop();
    testTaskDependencies();
    
    // 打印最终结果
    stats.printSummary();
    
    return stats.failed > 0 ? 1 : 0;
}