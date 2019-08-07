#include <fcntl.h>
#include <iostream>

#include "HttpServer.h"
#include "EventLoop.h"

const int StdinFd = 0;

HttpServer::HttpServer(EventLoop* loop, 
                       in_addr_t addr, 
                       uint16_t port,
                       int thread_num,
                       bool timer_on)
  : _loop(loop), 
    _addr(addr), 
    _port(port),
    _thread_num(thread_num),
    _timer_on(timer_on),
    _listen_socket(Socket::create()),
    _listen_trigger(_listen_socket.getFd()),
    _stdin_trigger(StdinFd),
    _loopthread_pool(loop),
    _idleFd(::open("/dev/null", O_RDONLY | O_CLOEXEC))
{
    // std::cout << "Listen Fd: " << _listen_socket.getFd() << std::endl;
    _listen_socket.setReuseAddr(true);
    _listen_socket.setReusePort(true);
    _listen_socket.setTcpNoDelay(true);
    _listen_socket.bind(addr, port);

    // _listen_trigger.setEvents(EPOLLIN | EPOLLPRI ); // 默认水平触发
    _listen_trigger.setEvents(EPOLLIN | EPOLLPRI | EPOLLET); // 边沿触发
    _listen_trigger.setHandler(std::bind(&HttpServer::handleRead, this));

    _stdin_trigger.setEvents(EPOLLIN);
    _stdin_trigger.setHandler(std::bind(&HttpServer::handleStdin, this));

    _loopthread_pool.setThreadNum(_thread_num);
    _loopthread_pool.enable();
}

HttpServer::~HttpServer()
{
    _loop->deleteTrigger(&_listen_trigger);
    ::close(_idleFd);
}

void HttpServer::listen() {
    _loop->addTrigger(&_listen_trigger);
    _loop->addTrigger(&_stdin_trigger);
    _listen_socket.listen();
    std::cout << "start running..." << std::endl;
}

void HttpServer::handleStdin() {
    char buf[16];
    ssize_t n = read(StdinFd, buf, 16);
    std::string command(buf, buf + n);
    // std::cout << n << " bytes command: " << command << std::endl;

    if(command == "quit\n") {
        this->quit();
    }
}

void HttpServer::handleRead() {
    struct sockaddr_in peeraddr;

    while(true) {
        int connfd = _listen_socket.accept(peeraddr);

        // std::cout << "HttpServer::handleRead(): " << std::endl;
        // std::cout << "      ListenFd(" << _listen_trigger.getFd() << " ";
        // std::cout << "Revents(" << _listen_trigger.getRevents() << ") accept ";
        // std::cout << "ConnFd = " << connfd << std::endl;

        if (connfd >= 0) {
            // std::cout << "      HttpServer::handleRead->newConnection" << std::endl;
            newConnection(connfd);
        }
        else {
            if (errno == EMFILE) { // 当文件描述符用完时采取的措施
                std::cerr << "Error: HttpServer::handleRead errno == EMFILE" << std::endl;
                ::close(_idleFd);
                _idleFd = ::accept(_listen_socket.getFd(), NULL, NULL);
                ::close(_idleFd);
                _idleFd = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
            }

            break;
        }
    }
}

void HttpServer::newConnection(int connfd) {
    // 从线程池中获取loop
    EventLoop* subloop = _loopthread_pool.getNextLoop();

    // 将conn加入loop
    ConnectionPtr conn = std::make_shared<Connection>(subloop, connfd, _timer_on);
    _connmap[connfd] = conn;
    conn->setCloseCallback(std::bind(&HttpServer::removeConnection, this, std::placeholders::_1));
    conn->enable();
}

/* 
 * 注意这里的实现,我们的目的有两个,首先在主线程的ConnectionMap中erase掉目标Connection,然后在子线程中释放掉目标
 * Connection,由于Connection由只能指针管理,所以我们需要保证全程至少拥有一个ConnectionPtr,仅仅在最后释放掉,即
 * 可完成任务.
 * 
 * 这里的整个过程为handleClose(子线程中运行)->removeConnection(子线程中运行)->removeConnectionInLoop(主线
 * 程的Loop中运行)->destroyConnection(子线程的Loop中运行);
 * 
 * 在removeConnectionInLoop中erase掉ConnectionMap中的目标Connection;
 * 在destroyConnection释放掉ConnectionPtr智能指针,即可释放整个Connection;
 * 
 * 这里的竞态情况:
 *  1.removeConnectionInLoop中主要为了避免对ConnectionMap的竞争,很可能多个线程使用ConnectionMap;
 *  2.destroyConnection中的fd描述符,有可能子线程正在使用此fd,而主线程即将close它,所以放在子线程的Loop中运行;
 * 
 */
void HttpServer::removeConnection(int fd) {
    _loop->runInLoop(std::bind(&HttpServer::removeConnectionInLoop, this, fd));
}


void HttpServer::removeConnectionInLoop(int fd) {
    ConnectionPtr conn = _connmap[fd];
    EventLoop* subLoop = conn->getLoop();
    this->_connmap.erase(fd);
    subLoop->runInLoop(std::bind(&Connection::destroyConnection, conn));
}

void HttpServer::quit() {
    _loop->quit();
}
