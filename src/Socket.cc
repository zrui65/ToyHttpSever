#include <string.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <assert.h>
#include <iostream>

#include "Socket.h"

int Socket::create() {
    //                     IP协议  字节流服务(TCP)     非阻塞                       TCP协议
    int sockfd = ::socket(PF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);

    if (sockfd < 0){
        std::cerr << "sockets::create" << std::endl;
    }

    return sockfd;
}

void Socket::bind(in_addr_t addr, uint16_t port) {
    struct sockaddr_in localaddr;
    memset(&localaddr, 0, sizeof(localaddr));
    localaddr.sin_family = AF_INET;
    localaddr.sin_port = htons(port);
    localaddr.sin_addr.s_addr = htonl(addr);

    int ret = ::bind(_socketfd, (sockaddr *)&localaddr, 
                     static_cast<socklen_t>(sizeof(struct sockaddr_in)));
    if (ret < 0)
    {
        std::cerr << "[Error] sockets::bind" << std::endl;
    }
}

void Socket::listen() {
    int ret = ::listen(_socketfd, SOMAXCONN);
    if (ret < 0)
    {
        std::cerr << "[Error] sockets::listen" << std::endl;
    }
}

int Socket::accept(struct sockaddr_in& peeraddr) {
    socklen_t peerlen = static_cast<socklen_t>(sizeof peeraddr);
    memset(&peeraddr, 0, sizeof peeraddr);
    int connfd = ::accept4(_socketfd, (struct sockaddr*)&peeraddr, 
                           &peerlen, SOCK_NONBLOCK | SOCK_CLOEXEC);

    return connfd;
}

int Socket::connect(const struct sockaddr* addr) {
    return ::connect(_socketfd, addr, 
                     static_cast<socklen_t>(sizeof(struct sockaddr_in6)));
}

ssize_t Socket::read(void *buf, size_t count) {
    return ::read(_socketfd, buf, count);
}

ssize_t Socket::write(const void *buf, size_t count) {
  return ::write(_socketfd, buf, count);
}

void Socket::shutdownWrite() {
    if(::shutdown(_socketfd, SHUT_WR) < 0) {
        std::cerr << "sockets::shutdownWrite" << std::endl;
    }
}

void Socket::setTcpNoDelay(bool on) {
    int optval = on ? 1 : 0;
    ::setsockopt(_socketfd, IPPROTO_TCP, TCP_NODELAY,
                &optval, static_cast<socklen_t>(sizeof optval));
}

void Socket::setReuseAddr(bool on) {
    int optval = on ? 1 : 0;
    ::setsockopt(_socketfd, SOL_SOCKET, SO_REUSEADDR,
                &optval, static_cast<socklen_t>(sizeof optval));
}

void Socket::setReusePort(bool on) {
    int optval = on ? 1 : 0;
    int ret = ::setsockopt(_socketfd, SOL_SOCKET, SO_REUSEPORT,
                            &optval, static_cast<socklen_t>(sizeof optval));
    if (ret < 0 && on) {
        std::cerr << "SO_REUSEPORT failed." << std::endl;
    }

}

void Socket::setKeepAlive(bool on) {
    int optval = on ? 1 : 0;
    ::setsockopt(_socketfd, SOL_SOCKET, SO_KEEPALIVE,
                &optval, static_cast<socklen_t>(sizeof optval));
}

void Socket::close() {
    // std::cout << "sockets::close" << _socketfd << std::endl;
    if (::close(_socketfd) < 0) {
        std::cerr << "[Error] sockets::close" << std::endl;
    }
}


