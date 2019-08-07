#include <sys/eventfd.h>
#include <string.h>
#include <iostream>

#include "EventLoop.h"
#include "TimerQueue.h"

const int EpollTimeMs = -1;
const int EventInitSize = 32;

int createEventfd() {
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0) {
        std::cerr << "[Error] createEventfd" << std::endl;
        abort();
    }
    return evtfd;
}

EventLoop::EventLoop()
:   _threadId(std::this_thread::get_id()),
    _events(EventInitSize),
    _epollfd(::epoll_create1(EPOLL_CLOEXEC)),
    _wakefd(createEventfd()),
    _wake_trigger(_wakefd.getFd()),
    _quit(false),
    _timer_queue(new TimerQueue(this))
{
    _wake_trigger.setEvents(EPOLLIN);
    _wake_trigger.setHandler(std::bind(&EventLoop::handleWakeup, this));
    this->addTrigger(&_wake_trigger);
}

EventLoop::~EventLoop() {
    this->deleteTrigger(&_wake_trigger);
    std::cout << "quit EventLoop!" << std::endl;
};

void EventLoop::wakeup() {
    uint64_t one = 1;
    ssize_t n = _wakefd.write(&one, sizeof one);
    if (n != sizeof one) {
        std::cerr << "EventLoop::wakeup() writes " << n << 
                    " bytes instead of 8" << std::endl;
    }
}

void EventLoop::handleWakeup() {
    uint64_t one = 1;
    ssize_t n = _wakefd.read(&one, sizeof one);
    if (n != sizeof one) {
        std::cerr << "EventLoop::handleWakeup() reads " << n << 
                    " bytes instead of 8" << std::endl;
    }
}

void EventLoop::quit() {
    _quit = true;
    if (!isInLoopThread()) {
        wakeup();
    }
}

// 此函数内联会导致invalid use of incomplete type
void EventLoop::addTimer(TimerPtr timer) {
    _timer_queue->addTimer(timer);
}

// 将trigger通过event.data.ptr保存在内核中
void EventLoop::setTrigger(epoll_event& event, Trigger* trigger) {
    event.data.ptr = trigger;
}

// epoll_wait返回后,再通过event.data.ptr得到之前保存的trigger
Trigger* EventLoop::getTrigger(const epoll_event& event) {
    return static_cast<Trigger*>(event.data.ptr);
}

void EventLoop::epollCtrl(Trigger* trigger, int commond) {
    epoll_event event;
    memset(&event, 0, sizeof event);
    int fd = trigger->getFd();
    event.events = trigger->getEvents();
    setTrigger(event, trigger);
    ::epoll_ctl(_epollfd.getFd(), commond, fd, &event);
}

// 当主线程新建Connection向子线程注册时,会调用子线程的此方法,
// 此时需要使用queueInLoop的方式调用.
void EventLoop::addTrigger(Trigger* trigger) {
    epollCtrl(trigger, EPOLL_CTL_ADD);
}

void EventLoop::modifyTrigger(Trigger* trigger) {
    epollCtrl(trigger, EPOLL_CTL_MOD);
}

void EventLoop::deleteTrigger(Trigger* trigger) {
    epollCtrl(trigger, EPOLL_CTL_DEL);
}

void EventLoop::fillActiveTriggers(size_t numEvents) {
    for (size_t i = 0; i < numEvents; ++i){
        Trigger* trigger = getTrigger(_events[i]);
        trigger->setRevents(_events[i].events);
        _active_trigger.push_back(trigger);
    }
}

void EventLoop::epollWait() {
    size_t numEvents = ::epoll_wait(_epollfd.getFd(),
                                    &*_events.begin(),
                                    static_cast<int>(_events.size()), // 每次接受的事件的最大值
                                    EpollTimeMs);
    if (numEvents > 0) {
        fillActiveTriggers(numEvents);
        if(numEvents == _events.size()) { //如果接受的事件达到了最大值
            _events.resize(_events.size() * 2); //就将最大值扩容一倍
        }
    }
}

void EventLoop::dealActiveTriggers() {
    for(Trigger* trigger : _active_trigger) {
        trigger->_handler();
    }
    // 处理完本轮事件之后要清空
    _active_trigger.clear();
}


void EventLoop::loop() {
    while(!_quit) {
        // std::cout << "start loop***********" << std::endl;
        this->epollWait();
        dealActiveTriggers();
        doPendingFunctors();
    }
}


// 以下这些函数是muduo网络库中对多线程任务的一个技巧实现;
// 一个线程向另一个线程添加任务均调用以下函数实现,所以下面的核心函数中
// 都会通过mutex进行保护,而上面的函数则必须运行在本线程中.

void EventLoop::runInLoop(Functor cb) {
    if(isInLoopThread()) { // 如果runInLoop被本线程调用,
        cb();              // 则直接运行.
    }
    else {                 // 如果本线程的runInLoop被其他线程调用,
        queueInLoop(std::move(cb)); // 则加入本线程的待运行队列,等待被调度.
    }
}

void EventLoop::queueInLoop(Functor cb) {
    {
        std::lock_guard<std::mutex> lock(_mtx);
        _pendingFunctors.push_back(std::move(cb));
    }

    if (!isInLoopThread()) {
        wakeup();
    }
}

void EventLoop::doPendingFunctors() {
    // 注意这里的实现技巧
    FunctorVector functors;

    {
        std::lock_guard<std::mutex> lock(_mtx);
        functors.swap(_pendingFunctors);
    }

    for (const Functor& functor : functors) {
        functor(); 
    }
}