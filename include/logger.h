#pragma once

#include<string>

//模板方法模式
#define LOG_INFO_(LogmsgFormat,...)\
    do\
    {\
        Logger &logger = Logger::instance();\
        logger.setLogLevel(INFO);\
        char buf[1024] = {0};\
        snprintf(buf,1024,LogmsgFormat,##__VA_ARGS__);\
        logger.log(buf);\
    }while(0)

#define LOG_ERROR_(LogmsgFormat,...)\
    do\
    {\
        Logger &logger = Logger::instance();\
        logger.setLogLevel(ERROR);\
        char buf[1024] = {0};\
        snprintf(buf,1024,LogmsgFormat,##__VA_ARGS__);\
        logger.log(buf);\
    }while(0)

//为了防止一些意想不到的错误，都使用do while(0)
//在这里面不能插注释！！！
//斜杠之后连空格都不能有
#define LOG_FATAL_(LogmsgFormat,...)\
    do{\
        Logger &logger = Logger::instance();\
        logger.setLogLevel(FATAL);\
        char buf[1024] = {0};\
        snprintf(buf,1024,LogmsgFormat,##__VA_ARGS__);\
        logger.log(buf);\
    }while(0)

#ifdef MUDEBUG
#define LOG_DEBUG(LogmsgFormat,...)\
    do{\
        Logger &logger = Logger::instance();\
        logger.setLogLevel(DEBUG);\
        char buf[1024] = {0};\
        snprintf(buf,1024,LogmsgFormat,##__VA_ARGS__);\
        logger.log(buf);\
    }while(0)
#else
    #define LOG_DEBUG_(LogmsgFormat,...)
#endif


//定义日志级别
enum Loglevel{
    INFO,   //普通信息
    ERROR,  //错误信息
    FATAL,  //core信息
    DEBUG,  //debug信息
};

class Logger{
public:
    //获取单例
    static Logger& instance();

    //设置日志级别
    void setLogLevel(int level);

    //写日志
    void log(std::string msg);

private:
    int loglevel_;
    //设置单例
    Logger(){}
};

