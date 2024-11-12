#include "heaptimer.h"

void HeapTimer::SwapNode_(size_t i,size_t j) {
    if(i < 0 || i>= heap_.size() || j < 0 || j>= heap_.size()) {
        throw std::invalid_argument("i or j 不在时间堆限定范围内");
    }
    std::swap(heap_[i],heap_[j]);
    ref_[heap_[i].id] = i;
    ref_[heap_[j].id] = j;
}

void HeapTimer::siftup_(size_t i) {
    if (i < 0 || i >= heap_.size()) {
        throw std::invalid_argument("i 不在时间堆限定范围内");
    }
    size_t j = (i-1)/2; // i的父亲
    while(j >= 0) {
        if(heap_[j] < heap_[i]){
            break;
        }
        SwapNode_(i,j);
        i = j;
        j = (i-1)/2;
    }
}

bool HeapTimer::siftdown_(size_t index, size_t n) {
    if (index < 0 || index >= heap_.size() || n < 0 || n >= heap_.size()){
        throw std::invalid_argument("index or j 不在时间堆限定范围内");
    }
    size_t i = index;
    size_t j = i * 2 + 1; // i的左孩子
    while(j < n) {
        if(j+1<n && heap_[j+1] < heap_[j]) {    //i的右孩子存在且比左孩子小，则更新j为右孩子
            j++;
        }
        if(heap_[i] < heap_[j]) {
            break;
        }
        SwapNode_(i,j);
        i = j;
        j = i * 2 + 1;
    }
    return i > index;
}

void HeapTimer::add(int id, int timeout, const TimeoutCallBack& cb) {
    if(id < 0){
        throw std::invalid_argument("id 不能小于0");
    }
    size_t i;
    if(ref_.count(id) == 0) {
        // 定时器id不在堆中，将其从堆尾插入，调整堆
        i = heap_.size();
        ref_[id] = i;
        heap_.push_back({id, Clock::now() + MS(timeout), cb});
        siftup_(i);
    } else {
        // 定时器id在堆中，取出并更新过期时间，调整堆
        i = ref_[id];
        heap_[i].expires = Clock::now() + MS(timeout);
        heap_[i].cb = cb;
        if(!siftdown_(i, heap_.size())) {
            siftup_(i);
        }
    }
}

void HeapTimer::del_(size_t index) {
    if(index < 0 || index >= heap_.size() || heap_.empty()){
        throw std::invalid_argument("index 越界或者堆为空");
    }
    size_t i = index;
    size_t n = heap_.size() -1; // 最后一个节点
    if(i < n) {     // 如果节点i不是最后一个节点
        SwapNode_(i,n);
        if(!siftdown_(i,n)){
            siftup_(i);
        }
    }
    ref_.erase(heap_.back().id);
    heap_.pop_back();
}

void HeapTimer::doWork(int id) {
    if(heap_.empty() || ref_.count(id) == 0){
        return;
    }
    size_t i = ref_[id];
    TimerNode node = heap_[i];
    node.cb();
    del_(i);
}

void HeapTimer::adjust(int id, int newExpires) {
    if(ref_.count(id) == 0) {
        throw std::invalid_argument("计时器id不在堆中");
    }
    int index = ref_[id];
    heap_[index].expires = Clock::now() + MS(newExpires);
    if(!siftdown_(index, heap_.size())){
        siftup_(index);
    }
}

void HeapTimer::clear() {
    ref_.clear();
    heap_.clear();
}

void HeapTimer::pop() {
    if(heap_.size() <0){
        throw std::underflow_error("堆中没有元素，无法pop堆顶元素");
    }
    del_(0);
}

void HeapTimer::tick() {
    while(!heap_.empty()) {
        TimerNode node = heap_.front();
        if(std::chrono::duration_cast<MS>(node.expires - Clock::now()).count() >0) {
            break;
        }
        node.cb();
        pop();
    }
}

int HeapTimer::GetNextTick() {
    tick();
    size_t res = -1;
    if(!heap_.empty()){
        res = std::chrono::duration_cast<MS>(heap_.front().expires - Clock::now()).count();
        if (res < 0){
            res = 0;
        }
    }
    return res;
}
