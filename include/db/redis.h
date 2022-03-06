#ifndef REDIS_H
#define REDIS_H

#include <hiredis/hiredis.h>
#include <string>
#include <vector>

using namespace std;

class Redis
{
public:
    Redis();
    ~Redis();

    // 连接redis服务器 
    bool connect(int port);

    string get_Command_s(string command); //return simple string
    int get_Command_i(string command);
    int get_Command_vs(vector<string>& res,string command);

private:
    redisContext *c;

};

#endif
