#include "LoopThreadPool.h"
#include "LoopThread.h"

LoopThreadPool::LoopThreadPool(EventLoop* loop)
:   _main_loop(loop), 
    _threads_num(0), 
    _next_num(0)
{}

LoopThreadPool::~LoopThreadPool() {}

void LoopThreadPool::enable() {
    for(int i = 0; i < _threads_num; ++i) {
        std::shared_ptr<LoopThread> thread = std::make_shared<LoopThread>();
        EventLoop* loop = thread->getLoop();
        _threads.push_back(thread);
        _loops.push_back(loop);
    }
}

void LoopThreadPool::setThreadNum(int n) {
    _threads_num = n;
}

EventLoop* LoopThreadPool::getNextLoop() {
    if(_threads_num == 0) return _main_loop;
    _next_num = (_next_num + 1) % _threads_num;
    return _loops[_next_num];
}
