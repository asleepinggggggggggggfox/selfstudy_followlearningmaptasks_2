# selfstudy_followmaptasks_2
目标：完成一个c++线程池
####一个线程池的参数大概如下：

```
public ThreadPoolExecutor (int corePoolSize
int maximumPoolSize,
long keepAliveTime,
TimeUnit unit,
BlockingQueue<Runnable> workQueue,
ThreadFactory threadFactory,
RejectedExecutionHandler handler) {
}
```