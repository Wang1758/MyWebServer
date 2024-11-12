#ifndef HEAP_TIMER_H
#define HEAP_TIMER_H

#include <queue>
#include <unordered_map>
#include <time.h>
#include <algorithm>
#include <arpa/inet.h>
#include <functional>
#include <assert.h>
#include <chrono>
#include "../log/log.h"

typedef std::function<void()> TimeoutCallBack;
typedef std::chrono::high_resolution_clock Clock;
typedef std::chrono::milliseconds MS;
typedef Clock::time_point TimeStamp;

// 定时器节点结构体
struct TimerNode    
{
    int id; // 标识定时器
    TimeStamp expires;  // 设置过期时间
    TimeoutCallBack cb; // 过期后触发的回调函数
    bool operator<(const TimerNode &t)
    {
        return expires < t.expires;
    }
};
class HeapTimer
{
public:
    HeapTimer() { heap_.reserve(64); }

    ~HeapTimer() { clear(); }
    // 调整编号为id的定时器的超时时间，并调整堆
    void adjust(int id, int newExpires);

    // 对编号为id的定时器进行添加或更新的操作，id不存在heap_中时将添加到堆中，否则更新过期时间并堆调整
    void add(int id, int timeOut, const TimeoutCallBack &cb);
    // 触发编号为id的定时器的回调函数，并删除该定时器
    void doWork(int id);
    // 清空heap_和ref_
    void clear();
    // 清理超时节点
    void tick();
    // 通过del_删除堆顶元素
    void pop();
    // 调用一次tick清理超时节点，返回剩余节点中最小的超时时间
    int GetNextTick();

private:
    // 从堆中删除节点index,同时更新ref_
    void del_(size_t index);

    // 对节点i进行上滤操作
    void siftup_(size_t i);
    // 对节点index进行下滤操作，直到节点n为止（不包括n），如果没有进行下滤则返回false，否则返回true
    bool siftdown_(size_t index, size_t n);

    // 交换节点i与节点j，并更新他们在ref_中的映射
    void SwapNode_(size_t i, size_t j);

    std::vector<TimerNode> heap_;
    // 定时器编号与堆中的节点的映射关系
    std::unordered_map<int, size_t> ref_;  
};

#endif // HEAP_TIMER_H