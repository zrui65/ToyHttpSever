
#pragma once

#include <vector>
#include <thread>

class EventLoop;
class LoopThread;

class LoopThreadPool {
public:
    LoopThreadPool(EventLoop*);
    ~LoopThreadPool();

    void enable();
    void setThreadNum(int);
    EventLoop* getNextLoop();

private:
    EventLoop* _main_loop;
    std::vector<EventLoop*> _loops;
    // 使用智能指针自动delete
    std::vector<std::shared_ptr<LoopThread>> _threads; 
    int _threads_num;
    int _next_num;

};

