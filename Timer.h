
#pragma once

#include <functional>
#include <sys/time.h>

class Timer {

public:
    Timer() :_invalid(false){}
    ~Timer() {}

    static int64_t now();
    static int64_t translateUsec(struct timeval);

    typedef std::function<void()> Function;

    int64_t key() {
        return _timeout;
    }

    bool isValid() {
        return !_invalid;
    }

    void setTime(int64_t timeout) {
        _timeout = timeout;
    }

    void setHandler(Function func) {
        _handler = func;
    }

    void deleteTimer() {
        _invalid = true;
    }

    void runHandler() {
        _handler();
    }

private:
    int64_t _timeout;
    Function _handler;
    bool _invalid;
};

