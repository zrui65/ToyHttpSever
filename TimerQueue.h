
#pragma once

#include <memory>
#include <deque>
#include <queue>
#include <sys/time.h>

#include "Timer.h"
#include "Trigger.h"

class EventLoop;
class Timer;

class TimerQueue {
public:
    typedef std::shared_ptr<Timer> TimerPtr;

    TimerQueue(EventLoop*);
    ~TimerQueue();

    void addTimer(TimerPtr);
    void handleExpiration();

private:
    void setNewTime(TimerPtr);

    EventLoop* _loop;
    int _timerfd;
    Trigger _trigger;

    std::function<bool(TimerPtr, TimerPtr)> comp = [](TimerPtr a, TimerPtr b){ 
        return a->key() > b->key(); 
    };

    std::priority_queue<TimerPtr, std::deque<TimerPtr>, decltype(comp)> _timer_list;

};

