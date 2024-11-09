#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <thread>
#include <assert.h>

class ThreadPool
{
private:
    struct Pool
    {
        std::mutex mtx_;
        std::condition_variable cond_;
        bool isClose;
        std::queue<std::function<void()>> tasks; // 任务队列，函数类型为void（）
    };
    std::shared_ptr<Pool> pool_;

public:
    ThreadPool() = default;
    ThreadPool(ThreadPool &&) = default;
    // 使用make_shared进行指针的初始化，会在一次内存分配中同时分配对象和控制块（原子操作），不会发生内存泄漏（发生了异常会正确释放内存）
    explicit ThreadPool(int threadCount) : pool_(std::make_shared<Pool>())
    {
        assert(threadCount > 0);
        for (int i = 0; i < threadCount; i++)
        {
            std::thread([pool = pool_](){
                std::unique_lock<std::mutex> locker(pool->mtx_);
                while(true){
                    if(!pool->tasks.empty()){
                        // 进入到这里一定是获取了锁的状态
                        auto task = std::move(pool->tasks.front()); // 左值变右值，资产转移
                        pool->tasks.pop();  
                        locker.unlock();    // 任务已经取出来了，提前解锁
                        task();
                        locker.lock();      // 马上需要取新的任务，获取锁
                    }else if(pool->isClose){
                        break;
                    }else{
                        pool->cond_.wait(locker);   //任务队列为空，等待任务来了后就被通知
                    }
                }
            }).detach();    //将子线程与主线程分离
        }
    }

    ~ThreadPool() {
        if(static_cast<bool>(pool_)){
            std::lock_guard<std::mutex> locker(pool_->mtx_);
            pool_->isClose = true;
        }
        pool_->cond_.notify_all();  //唤醒所有线程，关闭他们
    }

    template<class T>
    void AddTask(T&& task){
        {
            std::lock_guard<std::mutex> locker(pool_->mtx_);
            pool_->tasks.emplace(std::forward<T>(task));
        }
        pool_->cond_.notify_one();
    }
};

#endif