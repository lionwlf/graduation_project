#include <functional>
#include <string>

#include "server.h"
#include "service.h"

using namespace std;
using namespace placeholders;
        
Server::Server(EventLoop *loop,const InetAddress &listenAddr,const string &nameArg)
    : _server(loop, listenAddr, nameArg),
    _loop(loop)
{
    //注册连接回调
    _server.setConnectionCallback(std::bind(&Server::onConnection, this, _1));

    //注册消息回调
    _server.setMessageCallback(std::bind(&Server::onMessage, this, _1, _2, _3));

    //设置线程数
    _server.setThreadNum(5);
}

void Server::start()
{
    _server.start();
}

void Server::onMessage(const TcpConnectionPtr &conn, Buffer *buff, Timestamp time){
    string buf = buff->retrieveAllAsString();
    int msgid = stoi(buf.substr(0,2));

    //通过msgid获取业务回调，进行网络模块和任务模块之间的解耦合
    auto msgHandler = Service::instance()->getHandle(msgid);

    //回调消息绑定好的事件处理器，执行相应的业务处理
    msgHandler(conn,buf.substr(2),time);

    //成功解耦
}

void Server::onConnection(const TcpConnectionPtr &conn){
    if(!conn->connected()){ //用户断开连接
        Service::instance()->clientCloseException(conn);
        conn->shutdown();
    }
    else{
        cout<<"new connection"<<endl;
    }
}
