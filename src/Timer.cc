#include "Timer.h"

int64_t Timer::now() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000 + tv.tv_usec;
}

int64_t Timer::translateUsec(struct timeval time) {
    return time.tv_sec * 1000000 + time.tv_usec;
}

