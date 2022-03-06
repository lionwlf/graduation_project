#include "redis.h"

#include <iostream>

Redis::Redis() : c(nullptr){}

Redis::~Redis()
{
    if (c != nullptr)
    {
        redisFree(c);
    }
}

bool Redis::connect(int port)
{
    const char *hostname = "127.0.0.1";

    int port = port;

    struct timeval timeout = {1, 500000}; // 1.5 seconds

    c = redisConnectWithTimeout(hostname, port, timeout);

    if (c == NULL || c->err)
    {
        if (c)
        {
            std::cout<<"Redis Connection error: "<< c->errstr<<std::endl;
            //redisFree(c);
        }
        else
        {
            std::cout<<"Redis Connection error: can't allocate redis context"<<std::endl;
        }
        return false;
    }
    return true;
}

//for REDIS_REPLY_STRING 
string Redis::get_Command_s(string command){
    redisReply *reply = (redisReply *)redisCommand(c,command.c_str());
    if(reply->type == REDIS_REPLY_ERROR){
        std::cout<<reply->str<<std::endl;
        freeReplyObject(reply);
        return "";
    }
    else if(reply->type == REDIS_REPLY_NIL){
        freeReplyObject(reply);
        return "";
    }   
    else{
        string res = reply->str;
        freeReplyObject(reply);
        return res; //it's ok? ok
    }   
}

//for REDIS_REPLY_INTEGER
int Redis::get_Command_i(string command){
    redisReply *reply = (redisReply *)redisCommand(c,command.c_str());
    if(reply->type == REDIS_REPLY_ERROR){
        std::cout<<"redis command:"<<command<<"error:"<<reply->str<<std::endl;
        freeReplyObject(reply);
        return -1;
    }
    else if(reply->type == REDIS_REPLY_NIL){
        std::cout<<"redis command:"<<command<<"error:"<<reply->str<<std::endl;
        freeReplyObject(reply);
        return -1;
    } 
    else{
        int num = reply->integer;
        freeReplyObject(reply);
        return num;
    } 
}

//for REDIS_REPLY_INTEGER
int Redis::get_Command_vs(vector<string>& res,string command){
    redisReply *reply = (redisReply *)redisCommand(c,command.c_str());
    if(reply->type == REDIS_REPLY_ERROR){
        std::cout<<"redis command:"<<command<<"error:"<<reply->str<<std::endl;
        freeReplyObject(reply);
        return -1;
    }
    else if(reply->type == REDIS_REPLY_NIL){
        std::cout<<"redis command:"<<command<<"error:"<<reply->str<<std::endl;
        freeReplyObject(reply);
        return 0;
    } 
    else{
        int num = reply->elements;
        for(int i = 0;i<num;i++){
            res.push_back(reply->element[i]->str);
        }
        freeReplyObject(reply);
        return num;
    } 
}