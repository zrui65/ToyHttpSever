
#pragma once

#include <thread>
#include <mutex>
#include <condition_variable>

class EventLoop;

class LoopThread {
public:
    LoopThread();
    ~LoopThread();

    EventLoop* getLoop();
private:

    void threadTask();

    EventLoop* _loop;
    std::mutex _mtx;
    std::condition_variable _cond;
    std::thread _thread; // 位于主线程的堆中
};

