# selfstudy_followmaptasks_2
目标：完成一个c++线程池


#### 一个线程池的参数大概如下：

```
public ThreadPoolExecutor (
int corePoolSize                     线程池中保持常驻的核心线程数​
int maximumPoolSize,                 线程池允许创建的最大线程数​
long keepAliveTime,                  非核心线程空闲时的存活时间​
TimeUnit unit,                       keepAliveTime的时间单位​
BlockingQueue<Runnable> workQueue, ✔ 用于存放待处理任务的工作队列​
ThreadFactory threadFactory,         用于创建新线程的工厂​
RejectedExecutionHandler handler)    当线程池无法处理新任务时的拒绝策略
{
}
```
