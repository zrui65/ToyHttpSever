#include "LoopThread.h"
#include "EventLoop.h"

LoopThread::LoopThread()
:   _loop(nullptr),
    _mtx(),
    _cond(),
    // 需要放在互斥锁与条件变量之后,等待它们初始化好后在启动新线程
    _thread(std::bind(&LoopThread::threadTask, this)) 
{}

LoopThread::~LoopThread() {
    if(_loop) {
        _loop->quit();
    }
    _thread.join();
}

EventLoop* LoopThread::getLoop() {
    EventLoop* loop = nullptr;

    {
        std::unique_lock<std::mutex> lock(this->_mtx);
        this->_cond.wait(lock, [this](){ return _loop != nullptr; }); // 只有等到_loop被赋值之后才能返回
        loop = _loop;
    }

    return loop;
}


void LoopThread::threadTask() {
    EventLoop loop; // 位于子线程的用户栈中

    {
        std::unique_lock<std::mutex> lock(this->_mtx);
        _loop = &loop;
        this->_cond.notify_one();
    }

    loop.loop();
    _loop = nullptr;
}

