#include "ThreadPool.h" // 包含您提供的头文件
#include <iostream>
#include <vector>
#include <future>
#include <chrono>
#include <atomic>
#include <cassert>

// 辅助函数：模拟一个耗时任务
int slow_square(int x) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    return x * x;
}

// 测试1：基本功能测试
void test_basic_functionality() {
    std::cout << "=== 测试1：基本功能测试 ===" << std::endl;
    ThreadPool pool(4);

    // 测试无返回值任务
    auto future1 = pool.enqueue([]() {
        std::cout << "   无返回值任务在线程中执行。" << std::endl;
    });
    future1.get(); // 等待任务完成
    std::cout << "   ✓ 无返回值任务测试通过" << std::endl;

    // 测试有返回值任务
    auto future2 = pool.enqueue([]() -> int {
        return 2024;
    });
    assert(future2.get() == 2024);
    std::cout << "   ✓ 有返回值任务测试通过" << std::endl;

    // 测试传参任务
    int base = 10;
    auto future3 = pool.enqueue(slow_square, base);
    assert(future3.get() == 100);
    std::cout << "   ✓ 带参数任务测试通过" << std::endl;

    std::cout << "✓ 基本功能测试全部通过\n" << std::endl;
}

// 测试2：并发安全测试
void test_concurrent_safety() {
    std::cout << "=== 测试2：并发安全测试 ===" << std::endl;
    ThreadPool pool(4);
    std::atomic<int> counter(0);
    const int TOTAL_TASKS = 1000;
    std::vector<std::future<void>> futures;

    // 提交大量自增任务，检验计数器是否准确
    for (int i = 0; i < TOTAL_TASKS; ++i) {
        futures.push_back(pool.enqueue([&counter]() {
            counter.fetch_add(1, std::memory_order_relaxed);
        }));
    }
    // 等待所有任务完成
    for (auto& fut : futures) {
        fut.get();
    }
    assert(counter == TOTAL_TASKS);
    std::cout << "   计数器期望值: " << TOTAL_TASKS << ", 实际值: " << counter.load() << std::endl;
    std::cout << "✓ 并发安全测试通过\n" << std::endl;
}

// 测试3：性能与压力测试
void test_performance_pressure() {
    std::cout << "=== 测试3：性能与压力测试 ===" << std::endl;
    ThreadPool pool(8); // 使用较多线程来应对压力测试
    const int NUM_TASKS = 5000;
    std::vector<std::future<int>> futures;
    futures.reserve(NUM_TASKS);

    auto start_time = std::chrono::high_resolution_clock::now();

    // 提交大量计算任务
    for (int i = 0; i < NUM_TASKS; ++i) {
        futures.push_back(pool.enqueue([i]() -> int {
            return i * i;
        }));
    }

    // 验证所有结果
    for (int i = 0; i < NUM_TASKS; ++i) {
        assert(futures[i].get() == i * i);
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    std::cout << "   完成 " << NUM_TASKS << " 个任务耗时: " << duration.count() << " 毫秒" << std::endl;
    std::cout << "✓ 性能与压力测试通过\n" << std::endl;
}

// 测试4：状态监控测试
void test_status_monitoring() {
    std::cout << "=== 测试4：状态监控测试 ===" << std::endl;
    ThreadPool pool(3);

    std::cout << "   初始状态 - 总线程数: " << pool.threadsize()
              << ", 空闲线程数: " << pool.freethreadsize()
              << ", 队列任务数: " << pool.workqueuesize() << std::endl;

    // 提交一些耗时任务，观察状态变化
    std::vector<std::future<void>> futures;
    for (int i = 0; i < 5; ++i) {
        futures.push_back(pool.enqueue([]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }));
        std::this_thread::sleep_for(std::chrono::milliseconds(50)); // 稍作延迟，模拟任务提交间隔
        std::cout << "   提交任务后 - 总线程数: " << pool.threadsize()
                  << ", 空闲线程数: " << pool.freethreadsize()
                  << ", 队列任务数: " << pool.workqueuesize() << std::endl;
    }

    // 等待所有任务完成
    for (auto& fut : futures) {
        fut.get();
    }
    std::cout << "   任务完成后 - 总线程数: " << pool.threadsize()
              << ", 空闲线程数: " << pool.freethreadsize()
              << ", 队列任务数: " << pool.workqueuesize() << std::endl;
    std::cout << "✓ 状态监控测试通过\n" << std::endl;
}

// 测试5：异常处理测试
void test_exception_handling() {
    std::cout << "=== 测试5：异常处理测试 ===" << std::endl;
    ThreadPool pool(2);

    // 测试任务中抛出异常是否能通过future正确捕获
    auto future = pool.enqueue([]() -> int {
        throw std::runtime_error("这是一个在任务中抛出的测试异常");
        return 0;
    });

    try {
        future.get(); // 这里应该会抛出异常
        assert(false && "不应执行到此，应已捕获异常。"); // 如果执行到这里，说明测试失败
    } catch (const std::exception& e) {
        std::cout << "   成功捕获到任务抛出的异常: " << e.what() << std::endl;
    }
    std::cout << "✓ 异常处理测试通过\n" << std::endl;
}

// 测试6：线程池停止行为测试
void test_stop_behavior() {
    std::cout << "=== 测试6：线程池停止行为测试 ===" << std::endl;
    {
        ThreadPool pool(2);

        // 提交一个任务并确保它开始执行
        auto future = pool.enqueue([]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            std::cout << "   任务在线程池停止前已执行完毕。" << std::endl;
        });
        future.get(); // 等待这个任务完成

        std::cout << "   线程池即将析构..." << std::endl;
    } // pool在此作用域结束时析构，会调用其析构函数
    std::cout << "   线程池已安全析构。" << std::endl;
    std::cout << "✓ 线程池停止行为测试通过\n" << std::endl;
}

int main() {
    std::cout << "开始执行线程池全面测试...\n" << std::endl;

    try {
        test_basic_functionality();
        test_concurrent_safety();
        test_performance_pressure();
        test_status_monitoring();
        test_exception_handling();
        test_stop_behavior();

        std::cout << "🎉 恭喜！所有测试均通过，线程池功能正常。" << std::endl;
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "❌ 测试失败，发生异常: " << e.what() << std::endl;
        return 1;
    }
}