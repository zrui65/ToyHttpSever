#include <sys/timerfd.h>
#include <iostream>
#include "string.h"

#include "TimerQueue.h"
#include "EventLoop.h"

TimerQueue::TimerQueue(EventLoop* loop)
:   _loop(loop),
    _timerfd(::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC)),
    _trigger(_timerfd),
    _timer_list(comp) // 注意要使用lambda表达式初始化
{
    // _trigger.setEvents(EPOLLIN);
    _trigger.setEvents(EPOLLIN | EPOLLET);
    _trigger.setHandler(std::bind(&TimerQueue::handleExpiration, this));
    _loop->addTrigger(&_trigger);
}

TimerQueue::~TimerQueue() {
    _loop->deleteTrigger(&_trigger);
};

void TimerQueue::setNewTime(TimerPtr timer) {
    struct itimerspec newValue;
    memset(&newValue, 0, sizeof newValue);

    int64_t delta = timer->key() - Timer::now();

    newValue.it_value.tv_sec = delta / 1000000;
    newValue.it_value.tv_nsec = (delta % 1000000) * 1000;

    int ret = ::timerfd_settime(_timerfd, 0, &newValue, NULL);
    if (ret) {
        std::cerr << "[Error] timerfd_settime()" << std::endl;
    }
}

void TimerQueue::addTimer(TimerPtr timer) {
    // 如果队列为空或者新timer小于当前队列最小值,则重新设置定时器
    if(_timer_list.empty() || timer->key() < _timer_list.top()->key()) {
        setNewTime(timer);
    }

    _timer_list.push(timer);
}

void TimerQueue::handleExpiration(){

    uint64_t howmany;
    ::read(_timerfd, &howmany, sizeof howmany); // 当epoll为水平触发时,需要read_timerfd才能解除,否则会一直触发

    int64_t now_time = Timer::now();

    while(!_timer_list.empty()) {
        TimerPtr cur = _timer_list.top();
        
        if(cur->key() > now_time && cur->isValid()) {
            setNewTime(cur);
            break;
        }
        
        if(cur->isValid()) {
            cur->runHandler();
        }

        _timer_list.pop();
    }
}
