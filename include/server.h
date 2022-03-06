#ifndef SERVER_H_
#define SERVER_H_

#include<muduo/net/TcpServer.h>
#include<muduo/net/EventLoop.h>

#include<iostream>

using namespace std;
using namespace muduo;
using namespace muduo::net;


class Server{
private:
    TcpServer _server;
    EventLoop* _loop;

public:
    Server(EventLoop* loop,const InetAddress& listenAddr,const string& nameArg);
    
    void start();

private:
    void onConnection(const TcpConnectionPtr& conn);
    void onMessage(const TcpConnectionPtr& conn,Buffer* buff,Timestamp time);
};

#endif