#ifndef SERVICE_H_
#define SERVICE_H_

#include<muduo/net/TcpConnection.h>
#include<unordered_map>
#include<functional>
#include<mutex>
#include<string>

#include "msg.pb.h"
#include "db.h"
#include "redis.h"

using namespace std;
using namespace muduo;
using namespace muduo::net;

//处理消息的事件回调方法类型
using MsgHandler = std::function<void(const TcpConnectionPtr &conn,string s,Timestamp time)>;

//聊天服务器业务
class Service{
public:
    //单例模式
    static Service* instance();
    
    void login(const TcpConnectionPtr &conn,string s,Timestamp time);   //登录
    void reg(const TcpConnectionPtr &conn,string s,Timestamp time);     //注册
    void findpwd(const TcpConnectionPtr &conn,string s,Timestamp time); //找回密码
    
    //充值业务
    void recharge(const TcpConnectionPtr &conn,string s,Timestamp time);

    //查询两地车票
    void searchpick(const TcpConnectionPtr &conn,string s,Timestamp time);
    
    //订票
    void bookpick_S(const TcpConnectionPtr &conn,string s,Timestamp time);  //订一等票
    void bookpick_C(const TcpConnectionPtr &conn,string s,Timestamp time);  //订普通票
    void bookpick_ST(const TcpConnectionPtr &conn,string s,Timestamp time); //订站票
    
    //退票
    void cancelbook(const TcpConnectionPtr &conn,string s,Timestamp time);    

    //查询用户订单
    void personbook(const TcpConnectionPtr &conn,string s,Timestamp time);

    //获取消息对应的处理器
    MsgHandler getHandle(int msgid);

    // 处理注销业务
    void loginout(const TcpConnectionPtr &conn, string s, Timestamp time);

    void getcitymodel(const TcpConnectionPtr &conn, string s, Timestamp time);


    //处理客户端异常退出
    void clientCloseException(const TcpConnectionPtr &conn);
    //处理服务端异常退出
    void reset();
    
private:

    Service();

    //存储消息id和对应的处理方法
    //somebody needn't shared with other threads' resource
    unordered_map<int,MsgHandler> _msgHanderMap;
    unordered_map<int,TcpConnectionPtr> connMap;
    unordered_map<int,int*> slidewindowsMap;

    mutex _mutex;   //for connMap

private:
    bool check_serialnum(int id, int serialnum);
};

#endif
