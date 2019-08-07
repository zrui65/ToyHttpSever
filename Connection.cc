#include <iostream>
#include <string>
#include <errno.h>

#include "Connection.h"
#include "EventLoop.h"
#include "Timer.h"

Connection::Connection(EventLoop* loop, int fd, bool timer_on)
  : _loop(loop),
    _socketfd(fd),
    _trigger(fd),
    _timer_on(timer_on),
    _peer_shutdown(false)
{
    _socketfd.setTcpNoDelay(true);
    _trigger.setHandler(std::bind(&Connection::handleEvent, this));
    // _trigger.setEvents(EPOLLIN | EPOLLPRI);
    _trigger.setEvents(EPOLLIN | EPOLLPRI | EPOLLET);
}

Connection::~Connection() {}

void Connection::enable() {
    _loop->runInLoop(std::bind(&EventLoop::addTrigger, _loop, &_trigger));
}

void Connection::enableRead() {
    uint32_t events = _trigger.getEvents();
    events |= EPOLLIN;
    _trigger.setEvents(events);
    _loop->modifyTrigger(&_trigger);
}

void Connection::disableRead() {
    uint32_t events = _trigger.getEvents();
    events &= ~EPOLLIN;
    _trigger.setEvents(events);
    _loop->modifyTrigger(&_trigger);
}

void Connection::enableWrite() {
    uint32_t events = _trigger.getEvents();
    events |= EPOLLOUT;
    _trigger.setEvents(events);
    _loop->modifyTrigger(&_trigger);
}

void Connection::disableWrite() {
    uint32_t events = _trigger.getEvents();
    events &= ~EPOLLOUT;
    _trigger.setEvents(events);
    _loop->modifyTrigger(&_trigger);
}

void Connection::handleEvent() {
    uint32_t revents = _trigger.getRevents();
    // std::cout << "Connection::handleEvent" << std::endl;

    // 说明此TCP连接失效,收到对端发来的RST
    if ((revents & EPOLLHUP) && !(revents & EPOLLIN)) {
        handleClose();
    }

    // 对应的文件描述符发生错误
    if (revents & EPOLLERR) {
        handleError();
    }

    // "收到正常数据"或者"对端close或'shutdown写',收到Fin"
    if (revents & (EPOLLIN | EPOLLPRI | EPOLLRDHUP)) {
        handleRead();
    }

    // 可写
    if (revents & EPOLLOUT) {
        handleWrite();
    }
}

class HttpRequest;
void Connection::handleRead() {
    // 执行read
    // 如果可以读回数据,那么正常处理
    // 如果读回数据量为零,说明对端关闭了连接,触发了条件(EPOLLIN | EPOLLRDHUP) 本端也关闭

    // std::cout << "Connection::handleRead" << std::endl;

    ssize_t ret = readn();

    if(_peer_shutdown) {
        handleClose();
        return;
    }

    if(ret > 0) {
        size_t position = _http_process.process(_inbuffer, _outbuffer);

        if(!_outbuffer.empty()) {
            _inbuffer.erase(_inbuffer.begin(), _inbuffer.begin() + position);
            
            handleWrite();
            handleTimer();
        }
    }
    else {
        std::cerr << "[Error] readMessage " << errno << std::endl;
    }
    
}

void Connection::handleWrite() {
    // std::cout << "Connection::handleWrite" << std::endl;
    // std::cout << _outbuffer << std::endl;
    if(writen() < 0) { // 写操作出错
        handleClose();
    } 
    else if(_outbuffer.size() > 0) {
        enableWrite();
    }
    else {
        disableWrite();
    }
}

void Connection::handleError() {
    std::cerr << "[Error] Connection::handleError" << std::endl;
}

void Connection::handleTimer() {
    if(_http_process.isKeepLive()) {

        if(!_timer_on) return;

        if(_old_timer.get()) {
            _old_timer->deleteTimer();
        }

        _old_timer = std::make_shared<Timer>();
        _old_timer->setTime(Timer::now() + KEEP_ALIVE_TIME);
        _old_timer->setHandler(std::bind(&Connection::handleClose, this));

        _loop->addTimer(_old_timer);
    }
    else {
        handleClose();
    }

}

// 此函数有一个隐藏的参数this,将被传入一个ConnectionPtr智能指针作为局部变量,最后释放,即可释放掉整个Connection;
void Connection::destroyConnection() {}

// 关闭此Connection的唯一入口
// close时机:有可能在handleEvent中关闭,有可能在定时器中关闭;
// 有一种情况是,先在定时器中设置关闭,后在handleEvent中关闭,导致定时器触发时操作已经被关闭的Connection发生错误;
void Connection::handleClose() {
    // std::cout << "Connection::handleClose " << _socketfd.getFd() << std::endl;
    _loop->deleteTrigger(&_trigger);
    closeCallback(_socketfd.getFd());

    // 无论哪种方式调用的close函数,只需要删除定时器即可.
    if(_old_timer.get() != nullptr) {
        _old_timer->deleteTimer();
    }
}

ssize_t Connection::readn() {
    const int buff_size = 1024;
    ssize_t nread = 0;
    ssize_t readSum = 0;

    int fd = _trigger.getFd();
    char buff[buff_size];
    while (true) {

        if ((nread = ::read(fd, buff, buff_size)) < 0) {
            if (errno == EINTR) {
                continue;
            }
            else if (errno == EAGAIN) {
                return readSum;
            }  
            else {
                return -1;
            }
        }
        else if (nread == 0) { // 返回零说明对端关闭连接
            _peer_shutdown = true;
            break;
        }
        readSum += nread;
        _inbuffer.append(buff, buff + nread);
    }
    return readSum;
}

ssize_t Connection::writen() {
    size_t nleft = _outbuffer.size();
    ssize_t nwritten = 0;
    ssize_t writeSum = 0;

    const char *ptr = _outbuffer.c_str();
    int fd = _trigger.getFd();

    while (nleft > 0) {
        if((nwritten = write(fd, ptr, nleft)) <= 0) {
            if(nwritten < 0) {
                if (errno == EINTR) { // 如果errno == EINTR
                    nwritten = 0;     // 则一直重试写操作
                    continue;
                }
                else if (errno == EAGAIN) // 如果errno == EAGAIN
                    break;                // 则说明写缓冲区已满,返回并等待可写
                else
                    return -1;            // 写操作出错
            }
        }
        writeSum += nwritten;
        nleft -= nwritten;
        ptr += nwritten;
    }
    _outbuffer.erase(_outbuffer.begin(), _outbuffer.begin() + writeSum);
    return writeSum;
}
