// extreme_stress_test_fixed.cpp
#include "ThreadPool.hpp"
// extreme_stress_test_combined.cpp
#include "ThreadPool.hpp" // 请确保包含你的ThreadPool头文件
#include <iostream>
#include <atomic>
#include <vector>
#include <future>
#include <chrono>
#include <random>
#include <algorithm>
#include <mutex>
#include <condition_variable>
#include <cassert>

// ==========================================
// 测试1：千万级任务洪峰测试
// ==========================================
void testMillionTaskFlood() {
    std::cout << "=== 💥 千万级任务洪峰测试 ===" << std::endl;
    std::cout << "目标：1000万任务，检验内存管理和调度极限" << std::endl;
    
    const size_t num_tasks = 10000000;
    ThreadPool pool(16, 64, std::chrono::milliseconds(1000));
    
    std::atomic<long> completed_tasks(0);
    std::atomic<long long> total_execution_time(0);
    std::vector<std::future<long long>> futures;
    futures.reserve(100000);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 100);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // 分批提交策略，避免内存爆炸
    for (size_t i = 0; i < num_tasks; ++i) {
        if (i % 100000 == 0 && i > 0) {
            std::cout << "已提交 " << i << " 个任务..." << std::endl;
            for (auto& f : futures) { f.get(); }
            futures.clear();
        }
        
        futures.push_back(pool.submit([&completed_tasks, &total_execution_time, i, &dis, &gen]() -> long long {
            auto task_start = std::chrono::high_resolution_clock::now();
            
            // 混合任务类型
            int task_type = dis(gen) % 3;
            long long result = 0;
            
switch(task_type) {
    case 0: { // 计算密集型
        for (int j = 0; j < 5000; ++j) { result += j % 100; }
        break;
    }
    case 1: { // 内存操作
        std::vector<int> temp(1000);
        for (size_t k = 0; k < temp.size(); ++k) { temp[k] = k * i; }
        break;
    }
    default: { // 轻量计算
        result = i * 2;
        break;
    }
}
            
            auto task_end = std::chrono::high_resolution_clock::now();
            long long duration = std::chrono::duration_cast<std::chrono::microseconds>(
                task_end - task_start).count();
            
            total_execution_time += duration;
            completed_tasks++;
            return result;
        }));
    }
    
    // 等待最终批次
    for (auto& f : futures) { f.get(); }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto total_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "✓ 千万级洪峰测试完成" << std::endl;
    std::cout << "  总任务: " << num_tasks << " | 完成: " << completed_tasks.load() << std::endl;
    std::cout << "  总耗时: " << total_duration.count() << " ms" << std::endl;
    std::cout << "  吞吐量: " << (num_tasks * 1000.0 / total_duration.count()) << " tasks/sec" << std::endl;
    std::cout << "  平均耗时: " << (total_execution_time / num_tasks) << " μs" << std::endl;
}

// ==========================================
// 测试2：瞬时突发流量压力测试
// ==========================================
void testInstantBurstTraffic() {
    std::cout << "\n=== ⚡ 瞬时突发流量压力测试 ===" << std::endl;
    std::cout << "目标：3波5万并发，考验弹性扩缩容能力" << std::endl;
    
    ThreadPool pool(2, 100, std::chrono::milliseconds(500));
    
    const int BURST_SIZE = 50000;
    const int BURST_COUNT = 3;
    std::atomic<int> total_completed(0);
    std::vector<std::chrono::milliseconds> burst_durations;
    
    for (int burst = 0; burst < BURST_COUNT; ++burst) {
        std::cout << "  突发波次 " << (burst + 1) << " - 提交 " << BURST_SIZE << " 任务..." << std::endl;
        
        std::vector<std::future<int>> futures;
        futures.reserve(BURST_SIZE);
        std::atomic<int> burst_completed(0);
        
        auto burst_start = std::chrono::high_resolution_clock::now();
        
        // 瞬时提交大量任务
        for (int i = 0; i < BURST_SIZE; ++i) {
            futures.push_back(pool.submit([i, &burst_completed, &total_completed]() -> int {
                std::this_thread::sleep_for(std::chrono::microseconds(50));
                burst_completed++;
                total_completed++;
                return i * i;
            }));
        }
        
        // 等待本波次完成
        for (auto& f : futures) { f.get(); }
        
        auto burst_end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            burst_end - burst_start);
        burst_durations.push_back(duration);
        
        std::cout << "    波次 " << (burst + 1) << " 完成 | 耗时: " << duration.count() << " ms" << std::endl;
        std::cout << "    当前线程数: " << pool.get_thread_count() << std::endl;
        
        if (burst < BURST_COUNT - 1) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
    }
    
    long long total_time = 0;
    for (const auto& duration : burst_durations) {
        total_time += duration.count();
    }
    
    std::cout << "✓ 突发流量测试完成" << std::endl;
    std::cout << "  总处理任务: " << total_completed.load() << std::endl;
    std::cout << "  平均波次耗时: " << (total_time / BURST_COUNT) << " ms" << std::endl;
}

// ==========================================
// 测试3：激烈资源竞争压力测试
// ==========================================
void testIntenseResourceContention() {
    std::cout << "\n=== 🔥 激烈资源竞争压力测试 ===" << std::endl;
    std::cout << "目标：5万并发资源竞争，检验数据一致性" << std::endl;
    
    ThreadPool pool(8, 32, std::chrono::milliseconds(1000));
    
    const int CONCURRENT_ACCESS = 50000;
    std::atomic<int> shared_counter(0);
    std::atomic<int> read_operations(0);
    std::atomic<int> write_operations(0);
    
    // 共享资源
    std::vector<int> shared_data;
    std::mutex data_mutex;
    std::vector<std::future<void>> futures;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < CONCURRENT_ACCESS; ++i) {
        if (i % 5 == 0) { // 20%写操作
            futures.push_back(pool.submit([i, &shared_counter, &shared_data, &data_mutex, &write_operations]() {
                // 写操作：竞争修改共享数据
                {
                    std::unique_lock<std::mutex> lock(data_mutex);
                    shared_data.push_back(i);
                }
                shared_counter++;
                write_operations++;
            }));
        } else { // 80%读操作
            futures.push_back(pool.submit([i, &shared_counter, &shared_data, &data_mutex, &read_operations]() {
                // 读操作：竞争读取共享数据
                int local_sum = 0;
                {
                    std::unique_lock<std::mutex> lock(data_mutex);
                    if (!shared_data.empty()) {
                        local_sum = shared_data.size() % 100;
                    }
                }
                shared_counter++;
                read_operations++;
            }));
        }
    }
    
    // 等待所有竞争操作完成
    for (auto& f : futures) { f.get(); }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // 验证一致性
    bool counter_ok = (shared_counter == CONCURRENT_ACCESS);
    bool data_ok = (shared_data.size() == static_cast<size_t>(CONCURRENT_ACCESS / 5));
    
    std::cout << "✓ 资源竞争测试完成" << std::endl;
    std::cout << "  耗时: " << duration.count() << " ms" << std::endl;
    std::cout << "  原子计数器: " << (counter_ok ? "通过" : "失败") << " (" 
              << shared_counter << "/" << CONCURRENT_ACCESS << ")" << std::endl;
    std::cout << "  共享数据: " << (data_ok ? "通过" : "失败") << " (" 
              << shared_data.size() << "/" << (CONCURRENT_ACCESS / 5) << ")" << std::endl;
    std::cout << "  读操作: " << read_operations << " | 写操作: " << write_operations << std::endl;
    
    assert(counter_ok);
    assert(data_ok);
}

// ==========================================
// 测试4：边界与异常稳健性测试
// ==========================================
#include <stdexcept>

void testBoundaryAndRobustness() {
    std::cout << "\n=== 🌀 边界与异常稳健性测试 ===" << std::endl;
    std::cout << "目标：混合异常、极速任务，检验容错能力" << std::endl;
    
    ThreadPool pool(4, 16, std::chrono::milliseconds(500));
    std::vector<std::future<int>> futures;
    std::atomic<int> normal_completed(0);
    std::atomic<int> exception_caught(0);
    
    // 测试1: 异常任务处理
    std::cout << "  测试异常任务处理..." << std::endl;
    auto exception_future = pool.submit([]() -> int {
        throw std::runtime_error("模拟任务执行异常");
        return 42;
    });
    
    bool exception_handled = false;
    try {
        exception_future.get();
    } catch (const std::exception& e) {
        exception_handled = true;
        exception_caught++;
        std::cout << "    ✓ 异常捕获: " << e.what() << std::endl;
    }
    assert(exception_handled);
    
    // 测试2: 大量极速任务
    std::cout << "  测试大量极速任务..." << std::endl;
    const int FAST_TASKS = 10000;
    
    for (int i = 0; i < FAST_TASKS; ++i) {
        futures.push_back(pool.submit([i, &normal_completed]() -> int {
            normal_completed++;
            return i * i;
        }));
    }
    
    // 验证结果
    for (size_t i = 0; i < futures.size(); ++i) {
        int result = futures[i].get();
        assert(result == static_cast<int>(i) * static_cast<int>(i));
    }
    
    // 测试3: 混合正常和异常任务
    std::cout << "  测试混合正常/异常任务..." << std::endl;
    std::vector<std::future<int>> mixed_futures;
    const int MIXED_TASKS = 5000;
    
    for (int i = 0; i < MIXED_TASKS; ++i) {
        if (i % 7 == 0) {
            mixed_futures.push_back(pool.submit([]() -> int {
                throw std::logic_error("随机异常任务");
                return -1;
            }));
        } else {
            mixed_futures.push_back(pool.submit([i, &normal_completed]() -> int {
                normal_completed++;
                return i * 2;
            }));
        }
    }
    
    // 处理混合结果
    int mixed_success = 0;
    int mixed_failed = 0;
    
    for (auto& f : mixed_futures) {
        try {
            int result = f.get();
            mixed_success++;
        } catch (const std::exception& e) {
            mixed_failed++;
            exception_caught++;
        }
    }
    
    std::cout << "✓ 边界与异常测试完成" << std::endl;
    std::cout << "  正常任务完成: " << normal_completed.load() << std::endl;
    std::cout << "  异常任务捕获: " << exception_caught.load() << std::endl;
    std::cout << "  混合任务 - 成功: " << mixed_success << " | 失败: " << mixed_failed << std::endl;
}

// ==========================================
// 主测试函数
// ==========================================
int main() {
    std::cout << "==========================================" << std::endl;
    std::cout << "   C++11 线程池极限压力测试套件" << std::endl;
    std::cout << "==========================================" << std::endl;
    std::cout << "系统硬件并发数: " << std::thread::hardware_concurrency() << std::endl;
    std::cout << "警告：此测试将极大消耗系统资源，请确保系统稳定" << std::endl;
    std::cout << "按 Ctrl+C 可中断测试" << std::endl;
    
    auto global_start = std::chrono::high_resolution_clock::now();
    
    try {
        testMillionTaskFlood();
        testInstantBurstTraffic();
        testIntenseResourceContention();
        testBoundaryAndRobustness();
        
        auto global_end = std::chrono::high_resolution_clock::now();
        auto total_duration = std::chrono::duration_cast<std::chrono::seconds>(
            global_end - global_start);
        
        std::cout << "\n==========================================" << std::endl;
        std::cout << "   🎉 所有极限压力测试通过！" << std::endl;
        std::cout << "   总耗时: " << total_duration.count() << " 秒" << std::endl;
        std::cout << "   线程池在极端条件下表现稳健！" << std::endl;
        std::cout << "==========================================" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "\n❌ 测试失败: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}