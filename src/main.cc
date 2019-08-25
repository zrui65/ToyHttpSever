#include <signal.h>
#include <getopt.h>
#include <iostream>

#include "EventLoop.h"
#include "HttpServer.h"

/*
 * 当对端close连接后,本端依然向其write数据,本端将收到RST响应,如果仍要write数据,
 * 系统将给本进程发送SIGPIPE,此信号默认终止进程;
 * 此函数是将此信号的处理方式改为忽略,防止上述事件终止进程;
 */
void ignoreSigPipe() {
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = 0;
    sigaction(SIGPIPE, &sa, NULL);
}

static void usage(void) {
    std::cout << "ToyHttpServer [option]..." << std::endl;
    std::cout << "  -h             用法." << std::endl;
    std::cout << "  -t <num>       线程池数量." << std::endl;
    std::cout << "  -p <num>       监听端口." << std::endl;
    std::cout << "  -T <on/off>    <打开/关闭>定时器." << std::endl;
}

static void print_configuration(int thread_num, int port, bool timer_on) {
    std::cout << "ToyHttpServer Configuration" << std::endl;
    std::cout << "  线程池数量  : " << thread_num << std::endl;
    std::cout << "  监听端口    : " << port << std::endl;
    std::cout << "  定时器      : " << (timer_on ? "on" : "off") << std::endl;
}

int main(int argc, char *argv[]) {
    int opt = 0;
    int thread_num = 0;
    uint16_t port = 80;
    bool timer_on = true;

    // 解析输入参数
    while((opt = getopt(argc, argv, "ht:p:T:")) != EOF ) {
        std::string argstr(optarg);
        switch(opt)
        {
            case 'h': 
                usage();
                return 0;
                break;
            case 't': 
                thread_num = std::stoi(argstr);
                break;
            case 'p': 
                port = static_cast<uint16_t>(std::stoi(argstr));
                break;
            case 'T':
                if(argstr == "on") {
                    timer_on = true;
                }
                if(argstr == "off") {
                    timer_on = false;
                }
                break;
        }
    }

    // 输出配置
    print_configuration(thread_num, port, timer_on);

    // 忽略SIGPIPE信号
    ignoreSigPipe();

    EventLoop loop;
    HttpServer HttpServer(&loop, INADDR_ANY, port, thread_num, timer_on);
    HttpServer.listen();

    loop.loop();

    return 0;
}

