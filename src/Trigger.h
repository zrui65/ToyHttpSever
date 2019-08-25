
#pragma once

#include <functional>

class Trigger {
public:
    Trigger(int);
    ~Trigger();
    typedef std::function<void(void)> HandleFunc;
    void setHandler(HandleFunc handler) {
        _handler = handler;
    }

    int getFd() { return _fd; }
    uint32_t getEvents() { return _events; }
    uint32_t getRevents() { return _revents; }
    void setEvents(uint32_t events) { _events = events; }
    void setRevents(uint32_t revents) { _revents = revents; }

    HandleFunc _handler;

private:
    int _fd;
    uint32_t _events;
    uint32_t _revents;
    
};

