
#pragma once

#include <sys/epoll.h>
#include <mutex>
#include <functional>
#include <map>
#include <vector>
#include <thread>

#include "Trigger.h"
#include "Socket.h"

class Timer;
class TimerQueue;

class EventLoop {

public:
    typedef std::vector<struct epoll_event> EventVector;
    typedef std::vector<Trigger*> TriggerVector;
    typedef std::function<void(void)> Functor;
    typedef std::vector<Functor> FunctorVector;
    typedef std::shared_ptr<Timer> TimerPtr;

    EventLoop();
    ~EventLoop();

    void loop();
    void addTrigger(Trigger*);
    void modifyTrigger(Trigger*);
    void deleteTrigger(Trigger*);

    void runInLoop(Functor);
    void queueInLoop(Functor);
    void quit();
    void addTimer(TimerPtr);
    
private:

    void setTrigger(epoll_event&, Trigger*);
    Trigger* getTrigger(const epoll_event&);
    void epollCtrl(Trigger*, int);
    void epollWait();
    void fillActiveTriggers(size_t);
    void dealActiveTriggers();
    void doPendingFunctors();
    void wakeup();
    void handleWakeup();

    bool isInLoopThread() {
        return _threadId == std::this_thread::get_id();
        return false;
    }
    
    
    const std::thread::id _threadId;
    EventVector _events; // epoll_wait返回的事件
    Socket _epollfd;
    Socket _wakefd; // eventfd描述符,用于唤醒阻塞在epoll中的本线程
    Trigger _wake_trigger;
    bool _quit;
    std::shared_ptr<TimerQueue> _timer_queue;

    std::mutex _mtx;
    TriggerVector _active_trigger;
    FunctorVector _pendingFunctors;

};

