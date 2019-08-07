
#pragma once

#include <netinet/in.h>
#include <unistd.h>

class Socket {
public:
    Socket(int socketfd): _socketfd(socketfd) {}
    ~Socket() { 
        close(); 
        }

    int getFd() { return _socketfd; }

    static int create();

    void bind(in_addr_t, uint16_t);
    void listen();
    int accept(struct sockaddr_in&);
    int connect(const struct sockaddr*);

    ssize_t read(void*, size_t);
    ssize_t write(const void*, size_t);
    void shutdownWrite();

    void setTcpNoDelay(bool);
    void setReuseAddr(bool);
    void setReusePort(bool);
    void setKeepAlive(bool);

private:
    void close();

    int _socketfd;

};

