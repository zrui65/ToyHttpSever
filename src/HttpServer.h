
#pragma once

#include <map>
#include <netinet/in.h>

#include "Connection.h"
#include "LoopThreadPool.h"

class EventLoop;

class HttpServer {
public:

    HttpServer(EventLoop*, in_addr_t, uint16_t, int, bool);
    ~HttpServer();

    void listen();
private:

    typedef std::shared_ptr<Connection> ConnectionPtr;
    typedef std::map<int, ConnectionPtr> ConnectionMap;

    void handleRead();
    void handleStdin();
    void newConnection(int);
    void removeConnection(int);
    void removeConnectionInLoop(int);
    void quit();

    EventLoop* _loop;
    in_addr_t _addr;
    uint16_t _port;
    int _thread_num;
    bool _timer_on;
    Socket _listen_socket; // 析构函数中close套接字
    Trigger _listen_trigger; // 必须在_listen_socket之后
    Trigger _stdin_trigger;
    ConnectionMap _connmap;
    LoopThreadPool _loopthread_pool; // 必须位于最后,这样就会先析构_loopthread_pool再析构_connmap和其他
                                     // 这样是为了先退出所有线程,在析构所有Connection,否则会发生Trigger
                                     // 已经被析构了,但是其对应的fd发生事件并返回,在loop中执行
                                     // Trigger->handler时发生段错误.
    int _idleFd;

};

