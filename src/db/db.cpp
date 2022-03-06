#include "db.h"

MySQL::MySQL() { _conn = mysql_init(nullptr); }

// 释放数据库连接资源
MySQL::~MySQL()
{
    if (_conn != nullptr)
        mysql_close(_conn);
}

// 连接数据库
bool MySQL::connect()
{
    MYSQL *p = mysql_real_connect(_conn, server.c_str(), user.c_str(), password.c_str(), dbname.c_str(), 3306, nullptr, 0);
    if (p != nullptr)
    {
        mysql_query(_conn, "set names gbk");
    }
    else
    {
        cout << mysql_error(_conn) << endl;
        exit(-1);
    }
    return p;
}

// 更新操作
bool MySQL::update(string sql)
{
    if (mysql_query(_conn, sql.c_str()))
    {
        cout << mysql_error(_conn) << endl;
        return false;
    }
    else
        return true;
}

// 查询操作
MYSQL_RES *MySQL::query(string sql)
{
    if (mysql_query(_conn, sql.c_str()))
    {
        cout << mysql_error(_conn) << endl;
        return nullptr;
    }

    return mysql_use_result(_conn);
}

MYSQL *MySQL::getconnection()
{
    return _conn;
}
