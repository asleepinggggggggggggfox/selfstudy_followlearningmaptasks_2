
#include "ThreadPool.hpp"
#include <iostream>
#include <vector>
#include <future>
#include <atomic>

int main() {
    std::cout << "=== 修复版线程池测试 ===" << std::endl;
    
    try {
        // 创建线程池
        ThreadPool pool(2, 4);
        
        std::cout << "线程池创建成功，开始提交任务..." << std::endl;
        
        // 测试基本功能
        std::vector<std::future<int>> futures;
        std::atomic<int> sum{0};
        
        for (int i = 1; i <= 10; ++i) {
            auto future = pool.submit([i, &sum]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                int result = i * i;
                sum += result;
                std::cout << "任务 " << i << " 完成，结果: " << result << std::endl;
                return result;
            });
            futures.push_back(std::move(future));
        }
        
        std::cout << "所有任务提交完成，等待执行..." << std::endl;
        
        // 获取结果
        for (size_t i = 0; i < futures.size(); ++i) {
            int result = futures[i].get();
            std::cout << "获取任务 " << (i+1) << " 结果: " << result << std::endl;
        }
        
        std::cout << "总和: " << sum.load() << std::endl;
        std::cout << "测试完成！" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "测试失败: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}