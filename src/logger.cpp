#include"logger.h"
#include<iostream>


//单例模式 懒汉
Logger& Logger::instance(){
    static Logger logger;
    return logger;
}

//设置日志级别
void Logger::setLogLevel(int level){
    loglevel_ = level;
}

//写日志    [级别]：time：msg
void Logger::log(std::string msg){
    switch (loglevel_)
    {
    case INFO:
        std::cout<<"[INFO]:";
        break;
    case ERROR:
        std::cout<<"[ERRER]:";
        break;
    case FATAL:
        std::cout<<"[FATAL]:";
        break;
    case DEBUG:
        std::cout<<"[DEBUG]:";
        break;
    
    default:
        break;
    }

    std::cout<<msg<<std::endl;
}
