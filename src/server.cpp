#include <functional>
#include <string>
#include <time.h>
#include <math.h>

#include "server.h"
#include "service.h"

using namespace std;
using namespace placeholders;

//将 ip 转数字
long isplit(string str){
    int i = 3,pos = 0;
    long num = 0;
    while(i){
        cout<<str<<endl;
        pos = str.find('.');
        cout<<pos<<endl;
        cout<<str.substr(0,pos)<<endl;
        num = pow(10,pos)*num + stoi(str.substr(0,pos));
        str = str.substr(pos+1);
        i--;
        cout<<num<<endl;
    }
    num = pow(10,str.size())*num + stoi(str);
    return num;
}

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

bool Server::flow_control(const TcpConnectionPtr &conn){
    long ip = isplit(conn->peerAddress().toIp());
    
    //不是第一次请求咯
    time_t tm = time(NULL);
    if(flow_control_table.find(ip) != flow_control_table.end()){
        if(tm - flow_control_table[ip] >2){
            flow_control_table[ip] = tm;
            return true;
        }
        else{
            //如果要将 ip 加入黑名单，但是没必要，其实这种事情本来应该是路由器来做的
            return false;
        }
    }
    else{
        flow_control_table[ip] = tm;
        return true;
    }
}

void Server::onMessage(const TcpConnectionPtr &conn, Buffer *buff, Timestamp time){
    // 先做流量控制吧
    if(!flow_control(conn)){
        return;
    }

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
        cout<<"新链接ip："<< conn->peerAddress().toIp()<<endl;
        cout<<"new connection"<<endl;
    }
}
