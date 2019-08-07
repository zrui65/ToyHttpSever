
#pragma once

#include <sys/epoll.h>
#include <functional>
#include <memory>

#include "Trigger.h"
#include "Socket.h"
#include "HttpRequest.h"

class EventLoop;
class Timer;

class Connection {
public:
    Connection(EventLoop*, int, bool);
    ~Connection();

    typedef std::function<void(int)> HandleFunc;

    int getFd() { return _trigger.getFd(); }
    EventLoop* getLoop() { return _loop; }
    void enable();
    void setCloseCallback(HandleFunc cb) {
        closeCallback = cb;
    }
    void destroyConnection();
    
    void enableRead();
    void disableRead();
    void enableWrite();
    void disableWrite();

private:
    void handleRead();
    void handleWrite();
    void handleError();
    void handleTimer();
    void handleClose();
    void handleEvent();

    ssize_t readn();
    ssize_t writen();

    typedef std::shared_ptr<Timer> TimerPtr;

    HandleFunc closeCallback;

    // 虽然多个Connection共用一个EventLoop,但是这些Connection同时也
    // 处于一个线程中,所以不存在竞态,直接使用原生指针即可.
    EventLoop* _loop;
    Socket _socketfd;
    Trigger _trigger;
    bool _timer_on;

    std::string _inbuffer;
    std::string _outbuffer;
    
    HttpRequest _http_process;
    TimerPtr _old_timer;
    bool _peer_shutdown;
};

