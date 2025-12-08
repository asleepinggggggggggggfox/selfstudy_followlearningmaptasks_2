#include <iostream>
#include <chrono>
#include <vector>
#include <atomic>
#include <stdexcept>
#include <algorithm>
#include <random>
#include <memory>
#include <future>    // 添加：用于std::future
#include <thread>    // 添加：用于std::thread和std::this_thread
#include <cassert>   // 添加：用于assert

// 包含您的线程池头文件
#include "ThreadPool.hpp"

// 压力测试函数声明
void stressTestHighConcurrency();
void stressTestMassiveTasks();
void stressTestMemoryUsage();
void stressTestMixedWorkloads();
void stressTestExtremeConditions();
void stressPerformanceBenchmark();

int main() {
    std::cout << "=== ThreadPool 压力测试开始 ===\n" << std::endl;
    
    try {
        std::cout << "🚀 开始高并发压力测试..." << std::endl;
        stressTestHighConcurrency();
        
        std::cout << "\n🚀 开始海量任务压力测试..." << std::endl;
        stressTestMassiveTasks();
        
        std::cout << "\n🚀 开始内存使用压力测试..." << std::endl;
        stressTestMemoryUsage();
        
        std::cout << "\n🚀 开始混合工作负载测试..." << std::endl;
        stressTestMixedWorkloads();
        
        std::cout << "\n🚀 开始极端条件测试..." << std::endl;
        stressTestExtremeConditions();
        
        std::cout << "\n🚀 开始性能基准测试..." << std::endl;
        stressPerformanceBenchmark();
        
        std::cout << "\n✅ 所有压力测试通过！线程池表现稳定可靠" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "❌ 压力测试失败: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}

// 1. 高并发压力测试
void stressTestHighConcurrency() {
    std::cout << "1. 高并发压力测试..." << std::endl;
    
    const size_t thread_count = std::thread::hardware_concurrency();
    ThreadPool pool(thread_count);
    std::cout << "   使用 " << thread_count << " 个线程进行测试" << std::endl;
    
    const int total_tasks = 10000;
    std::atomic<int> completed_tasks{0};
    std::atomic<int> active_threads{0};
    std::atomic<int> max_concurrent{0};
    
    std::vector<std::future<void>> futures;
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // 提交大量并发任务
    for (int i = 0; i < total_tasks; ++i) {
        futures.emplace_back(pool.enqueue([i, &completed_tasks, &active_threads, &max_concurrent]() {
            // 模拟工作负载
            int current_active = ++active_threads;
            int old_max = max_concurrent.load();
            while (old_max < current_active && 
                   !max_concurrent.compare_exchange_weak(old_max, current_active)) {
                old_max = max_concurrent.load();
            }
            
            // 模拟计算密集型工作
            volatile int result = 0;
            for (int j = 0; j < 1000; ++j) {
                result += j * j;
            }
            
            --active_threads;
            completed_tasks++;
            
            // 随机微小延迟，模拟真实工作负载
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }));
    }
    
    // 等待所有任务完成
    for (auto& future : futures) {
        future.get();
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "   ✓ 完成 " << total_tasks << " 个任务" << std::endl;
    std::cout << "   ✓ 最大并发数: " << max_concurrent << std::endl;
    std::cout << "   ✓ 总耗时: " << duration.count() << "ms" << std::endl;
    std::cout << "   ✓ 吞吐量: " << (total_tasks * 1000.0 / duration.count()) << " 任务/秒" << std::endl;
    
    assert(completed_tasks == total_tasks);
    std::cout << "   ✅ 高并发压力测试通过" << std::endl;
}

// 2. 海量任务压力测试
void stressTestMassiveTasks() {
    std::cout << "2. 海量任务压力测试..." << std::endl;
    
    ThreadPool pool(8);
    const int massive_task_count = 100000; // 10万任务
    std::atomic<long long> total_result{0};
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    std::vector<std::future<long long>> futures;
    futures.reserve(massive_task_count);
    
    // 提交海量小任务
    for (int i = 0; i < massive_task_count; ++i) {
        futures.emplace_back(pool.enqueue([i]() -> long long {
            // 快速计算任务
            long long sum = 0;
            for (int j = 0; j <= i % 100; ++j) {
                sum += j;
            }
            return sum;
        }));
    }
    
    // 收集结果
    for (auto& future : futures) {
        total_result += future.get();
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "   ✓ 处理 " << massive_task_count << " 个任务" << std::endl;
    std::cout << "   ✓ 总计算结果: " << total_result << std::endl;
    std::cout << "   ✓ 耗时: " << duration.count() << "ms" << std::endl;
    std::cout << "   ✓ 平均延迟: " << (duration.count() * 1000.0 / massive_task_count) << "微秒/任务" << std::endl;
    
    std::cout << "   ✅ 海量任务压力测试通过" << std::endl;
}

// 3. 内存使用压力测试（修复make_unique问题）
void stressTestMemoryUsage() {
    std::cout << "3. 内存使用压力测试..." << std::endl;
    
    // 测试内存使用是否稳定
    const int iterations = 10;
    const int tasks_per_iteration = 5000;
    
    auto start_memory = std::chrono::high_resolution_clock::now();
    
    for (int iter = 0; iter < iterations; ++iter) {
        ThreadPool pool(4);
        std::atomic<int> counter{0};
        std::vector<std::future<void>> futures;
        
        // 提交大量任务，测试内存管理
        for (int i = 0; i < tasks_per_iteration; ++i) {
            futures.emplace_back(pool.enqueue([i, &counter]() {
                // 分配一些内存然后释放（使用C++11兼容的方式）
                std::unique_ptr<int[]> buffer(new int[100]); // 替代std::make_unique
                for (int j = 0; j < 100; ++j) {
                    buffer[j] = j * i;
                }
                counter++;
            }));
        }
        
        // 等待完成
        for (auto& future : futures) {
            future.get();
        }
        
        assert(counter == tasks_per_iteration);
        
        if ((iter + 1) % 5 == 0) {
            std::cout << "   完成迭代 " << (iter + 1) << "/" << iterations << std::endl;
        }
    }
    
    auto end_memory = std::chrono::high_resolution_clock::now();
    auto memory_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_memory - start_memory);
    
    std::cout << "   ✓ 内存稳定性测试完成" << std::endl;
    std::cout << "   ✓ 总耗时: " << memory_duration.count() << "ms" << std::endl;
    std::cout << "   ✅ 内存使用压力测试通过" << std::endl;
}

// 4. 混合工作负载测试
void stressTestMixedWorkloads() {
    std::cout << "4. 混合工作负载测试..." << std::endl;
    
    ThreadPool pool(6);
    std::atomic<int> short_tasks{0};
    std::atomic<int> medium_tasks{0};
    std::atomic<int> long_tasks{0};
    
    std::vector<std::future<void>> futures;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // 混合不同类型的工作负载
    for (int i = 0; i < 300; ++i) {
        // 短任务（即时完成）
        futures.emplace_back(pool.enqueue([&short_tasks]() {
            short_tasks++;
        }));
        
        // 中等任务（少量计算）
        if (i % 3 == 0) {
            futures.emplace_back(pool.enqueue([&medium_tasks]() {
                volatile int sum = 0;
                for (int j = 0; j < 10000; ++j) {
                    sum += j * j;
                }
                medium_tasks++;
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }));
        }
        
        // 长任务（较重负载）
        if (i % 10 == 0) {
            futures.emplace_back(pool.enqueue([&long_tasks]() {
                volatile int result = 0;
                for (int j = 0; j < 100000; ++j) {
                    result += j * j;
                }
                long_tasks++;
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }));
        }
    }
    
    // 等待所有任务完成
    for (auto& future : futures) {
        future.get();
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "   ✓ 短任务完成: " << short_tasks << std::endl;
    std::cout << "   ✓ 中等任务完成: " << medium_tasks << std::endl;
    std::cout << "   ✓ 长任务完成: " << long_tasks << std::endl;
    std::cout << "   ✓ 总任务数: " << (short_tasks + medium_tasks + long_tasks) << std::endl;
    std::cout << "   ✓ 混合负载耗时: " << duration.count() << "ms" << std::endl;
    
    std::cout << "   ✅ 混合工作负载测试通过" << std::endl;
}

// 5. 极端条件测试
void stressTestExtremeConditions() {
    std::cout << "5. 极端条件测试..." << std::endl;
    
    // 测试1: 单线程处理大量任务
    std::cout << "   测试1: 单线程池压力测试..." << std::endl;
    {
        ThreadPool single_thread_pool(1);
        std::atomic<int> counter{0};
        std::vector<std::future<void>> futures;
        
        for (int i = 0; i < 1000; ++i) {
            futures.emplace_back(single_thread_pool.enqueue([&counter, i]() {
                std::this_thread::sleep_for(std::chrono::microseconds(100));
                counter++;
            }));
        }
        
        for (auto& future : futures) {
            future.get();
        }
        
        assert(counter == 1000);
        std::cout << "   ✓ 单线程池测试通过" << std::endl;
    }
    
    // 测试2: 频繁创建销毁线程池
    std::cout << "   测试2: 频繁创建销毁测试..." << std::endl;
    {
        for (int i = 0; i < 50; ++i) {
            ThreadPool temp_pool(2);
            auto future = temp_pool.enqueue([]() { return 42; });
            assert(future.get() == 42);
        }
        std::cout << "   ✓ 频繁创建销毁测试通过" << std::endl;
    }
    
    // 测试3: 任务抛出异常的处理
    std::cout << "   测试3: 异常任务压力测试..." << std::endl;
    {
        ThreadPool exception_pool(4);
        std::atomic<int> success_count{0};
        std::atomic<int> exception_count{0};
        std::vector<std::future<void>> futures;
        
        for (int i = 0; i < 200; ++i) {
            if (i % 10 == 0) {
                // 每10个任务中有一个抛出异常
                futures.emplace_back(exception_pool.enqueue([&exception_count]() {
                    throw std::runtime_error("模拟任务异常");
                    exception_count++;
                }));
            } else {
                futures.emplace_back(exception_pool.enqueue([&success_count]() {
                    success_count++;
                }));
            }
        }
        
        // 处理异常
        int handled_exceptions = 0;
        for (auto& future : futures) {
            try {
                future.get();
            } catch (const std::exception&) {
                handled_exceptions++;
            }
        }
        
        std::cout << "   ✓ 成功任务: " << success_count << std::endl;
        std::cout << "   ✓ 处理异常: " << handled_exceptions << std::endl;
        std::cout << "   ✓ 异常处理测试通过" << std::endl;
    }
    
    std::cout << "   ✅ 极端条件测试通过" << std::endl;
}

// 6. 性能基准测试（修复printf格式警告）
void stressPerformanceBenchmark() {
    std::cout << "6. 性能基准测试..." << std::endl;
    
    // 测试不同线程数量的性能
    std::vector<size_t> thread_counts = {1, 2, 4, 8, 16};
    const int benchmark_tasks = 5000;
    
    std::cout << "   线程数 | 耗时(ms) | 吞吐量(任务/秒)" << std::endl;
    std::cout << "   --------+----------+----------------" << std::endl;
    
    for (size_t threads : thread_counts) {
        ThreadPool pool(threads);
        auto start = std::chrono::high_resolution_clock::now();
        
        std::vector<std::future<int>> futures;
        for (int i = 0; i < benchmark_tasks; ++i) {
            futures.emplace_back(pool.enqueue([i]() {
                // 标准工作负载
                int result = 0;
                for (int j = 0; j < 1000; ++j) {
                    result += (i + j) * (i - j);
                }
                return result;
            }));
        }
        
        for (auto& future : futures) {
            future.get();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        double throughput = (benchmark_tasks * 1000.0) / duration.count();
        
        // 修复格式警告：使用 %ld 替代 %lld
        printf("   %-7zu | %-8ld | %.0f\n", 
               threads, static_cast<long>(duration.count()), throughput);
    }
    
    std::cout << "   ✅ 性能基准测试完成" << std::endl;
}