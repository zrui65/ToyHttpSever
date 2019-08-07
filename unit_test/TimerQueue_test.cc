#include "EventLoop.h"
#include "HttpServer.h"
#include "Connection.h"

#include "Timer.h"
#include <memory>

int main() {
    EventLoop loop;

    typedef std::shared_ptr<Timer> TimerPtr;

    TimerPtr a = std::make_shared<Timer>();
    a->setTime(Timer::now() + 5000000);
    a->setHandler([](){std::cout << "5s" << std::endl;});
    loop.addTimer(a);

    TimerPtr b = std::make_shared<Timer>();
    b->setTime(Timer::now() + 2000000);
    b->setHandler([](){std::cout << "2s" << std::endl;});
    loop.addTimer(b);

    TimerPtr c = std::make_shared<Timer>();
    c->setTime(Timer::now() + 7000000);
    c->setHandler([](){std::cout << "7s" << std::endl;});
    loop.addTimer(c);

    a->deleteTimer();

    TimerPtr d = std::make_shared<Timer>();
    d->setTime(Timer::now() + 4000000);
    d->setHandler([](){std::cout << "4s" << std::endl;});
    loop.addTimer(d);

    loop.loop();
    
    return 0;
}
