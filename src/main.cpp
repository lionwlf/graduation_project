#include<iostream>
#include<signal.h>
#include "server.h"
#include "service.h"

using namespace std;

void resetHandler(int){
    Service::instance()->reset();
    exit(0);
}

int main(int argc, char **argv){
    // if (argc < 3){
    //     cerr << "command invalid! example: ./ChatServer 127.0.0.1 6000" << endl;
    //     exit(-1);
    // }

    // char *ip = argv[1];
    // uint16_t port = atoi(argv[2]);

    string sip = "192.168.190.133";
    const char* ip = sip.c_str();
    uint16_t port = 8000;

    //捕捉 Ctrl+C 信号
    signal(SIGINT,resetHandler);

    EventLoop loop;
    InetAddress addr(ip, port);
    Server server(&loop,addr,"Server");

    server.start();
    loop.loop();
    return 0;
}

