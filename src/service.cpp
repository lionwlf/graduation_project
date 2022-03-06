#include "service.h"

#include <time.h>
#include <iostream>
#include <cmath>
#include <vector>

//循环数组共10个位置
/*
    不正常的序列号分布为：
    m>l or m>r, m为插入位置，l为插入位置左侧，r为插入位置右侧

    m：为用户传入的序列号模除10而来

    当登记序列号于m位置时，该位置的m+1
*/
bool Service::check_serialnum(int id, int m)
{
    int l, r;
    if (m == 0)
    {
        l = 9;
        r = 1;
    }
    else if (m == 9)
    {
        l = 8;
        r = 0;
    }
    else
    {
        l = m - 1;
        r = m + 1;
    }

    int *serial = slidewindowsMap[id];
    if (serial[m] > serial[l] || serial[m] > serial[r])
    {
        return false;
    }
    else
    {
        // no need to lock,because one id per thread

        ++slidewindowsMap[id][m];
        return true;
    }
}

Service *Service::instance()
{
    static Service service;

    return &service;
}

//注册消息以及对应的回调操作
Service::Service()
{
    _msgHanderMap.insert({11, std::bind(&Service::login, this, _1, _2, _3)});
    _msgHanderMap.insert({12, std::bind(&Service::reg, this, _1, _2, _3)});
    _msgHanderMap.insert({13, std::bind(&Service::loginout, this, _1, _2, _3)});
    _msgHanderMap.insert({14, std::bind(&Service::recharge, this, _1, _2, _3)});
    _msgHanderMap.insert({15, std::bind(&Service::findpwd, this, _1, _2, _3)});
    _msgHanderMap.insert({16, std::bind(&Service::searchpick, this, _1, _2, _3)});
    _msgHanderMap.insert({17, std::bind(&Service::bookpick_S, this, _1, _2, _3)});
    _msgHanderMap.insert({18, std::bind(&Service::bookpick_C, this, _1, _2, _3)});
    _msgHanderMap.insert({19, std::bind(&Service::bookpick_ST, this, _1, _2, _3)});
    _msgHanderMap.insert({20, std::bind(&Service::cancelbook, this, _1, _2, _3)});
    _msgHanderMap.insert({21, std::bind(&Service::personbook, this, _1, _2, _3)});
    _msgHanderMap.insert({22, std::bind(&Service::getcitymodel, this, _1, _2, _3)});
}

//获取存储消息id和对应的处理方法
MsgHandler Service::getHandle(int msgid)
{
    //日志记录
    auto it = _msgHanderMap.find(msgid);
    if (it == _msgHanderMap.end())
    {
        //返回一个lambda表达式，返回一个默认的空处理器，防止业务挂掉，后可做平滑升级处理
        return [=](const TcpConnectionPtr &conn, string s, Timestamp time)
        {
            cout << __FILE__ << " " << __LINE__ << endl;
        };
    }
    else
    {
        return _msgHanderMap[msgid];
    }
}

void Service::login(const TcpConnectionPtr &conn, string s, Timestamp time)
{
    login_request login_;
    if (login_.ParseFromString(s))
    {
        int id_ = login_.id();
        string id = to_string(id_);
        int pwd = login_.pwd();

        login_reply reply;
        int msg;

        // just one handle? or like the mysql's roll trick is better? or make a pool?
        // you should make a test
        if (!_redis.connect(6379))
        {
            cout << "redis connection faild!!!" << endl;
            exit(-1);
        }

        // the id is not exists in redis and set the id into redis successfully
        if (_redis.get_Command_i("setnx id" + id + " 1"))
        {
            char sql1[24] = {};

            sprintf(sql1, "call login(%d)", id_);

            MySQL mysql;
            mysql.connect();
            MYSQL_RES *res = mysql.query(sql1);
            if (res != nullptr) // it returns a data set anyway,unless it's temporarily broken
            {
                MYSQL_ROW row = mysql_fetch_row(res);
                if (row != nullptr && pwd == stoi(row[0]))
                {
                    msg = stoi(row[1]);
                    mysql_free_result(res);

                    int slidewindows[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
                    {
                        lock_guard<mutex> lock(_mutex);
                        connMap.insert({id_, conn});
                        slidewindowsMap[id_] = slidewindows;
                    }
                }
                else
                {
                    mysql_free_result(res);
                    _redis.get_Command_i("del id" + id);

                    if (row == nullptr)
                    {
                        msg = 700000;
                    }
                    else
                    {
                        msg = 700001;
                    }
                }
            }
        }
        else
        {
            msg = 700002;
        }

        reply.set_msg(msg);
        string response = "11" + reply.SerializeAsString();
        conn->send(response);
    }
    else
    {
        cout << __FILE__ << " " << __LINE__ << endl;
    }
}

void Service::reg(const TcpConnectionPtr &conn, string s, Timestamp time)
{
    regists_request reg_;
    if (reg_.ParseFromString(s))
    {
        regists_reply reply;
        int pwd = reg_.pwd();
        long phone_num = reg_.phone_num();

        int msg;
        char sql[36] = {};
        sprintf(sql, "call reg(%d,%ld)", pwd, phone_num);

        MySQL mysql;
        mysql.connect();

        MYSQL_RES *res = mysql.query(sql);

        if (res != nullptr)
        {
            MYSQL_ROW row = mysql_fetch_row(res);
            if (row != nullptr)
            {
                msg = stoi(row[0]);
            }
            else
            {
                cout << __FILE__ << " " << __LINE__ << endl;
            }
            mysql_free_result(res);
        }
        reply.set_msg(msg);
        string response = "12" + reply.SerializeAsString();
        conn->send(response);
    }
    else
    {
        cout << __FILE__ << " " << __LINE__ << endl;
    }
}

void Service::loginout(const TcpConnectionPtr &conn, string s, Timestamp time)
{
    loginout_request loginout_;
    if (loginout_.ParseFromString(s))
    {
        int id = loginout_.id();

        _redis.get_Command_i("del id" + id);

        {
            lock_guard<mutex> lock(_mutex);
            connMap.erase(id);
            slidewindowsMap.erase(id);
        }
    }
    else
    {
        cout << __FILE__ << " " << __LINE__ << endl;
    }
}

void Service::recharge(const TcpConnectionPtr &conn, string s, Timestamp time)
{
    recharge_request recharge_;
    if (recharge_.ParseFromString(s))
    {
        int id = recharge_.id();
        int serial_num = recharge_.serial_num();

        if (check_serialnum(id, serial_num))
        {
            recharge_reply reply;
            int money = recharge_.money();

            char sql[32] = {};
            sprintf(sql, "call recharge(%d,%d)", id, money);

            MySQL mysql;
            mysql.connect();
            MYSQL_RES *res = mysql.query(sql);
            if (res != nullptr)
            {
                MYSQL_ROW row = mysql_fetch_row(res);
                if (row)
                {
                    reply.set_errno_id(stoi(row[0]));
                }
            }
            string response = "14" + reply.SerializeAsString();
            conn->send(response);
        }
    }
    else
    {
        cout << __FILE__ << " " << __LINE__ << endl;
    }
}

void Service::findpwd(const TcpConnectionPtr &conn, string s, Timestamp time)
{
    findpwd_request fpr;
    if (fpr.ParseFromString(s))
    {
        int id = fpr.id();
        int serial_num = fpr.serial_num();

        if (check_serialnum(id, serial_num))
        {
            long phone_num = fpr.phone_num();

            char sql[36] = {};
            sprintf(sql, "call findpwd(%d,%ld)", id, phone_num);

            findpwd_reply reply;
            MySQL mysql;
            mysql.connect();
            MYSQL_RES *res = mysql.query(sql);
            if (res != nullptr)
            {
                MYSQL_ROW row = mysql_fetch_row(res);
                if (row != nullptr)
                {
                    reply.set_msg(stoi(row[0]));
                }
                else
                {
                    reply.set_msg(700003);
                    cout << __FILE__ << " " << __LINE__ << endl;
                }
            }
            string response = "15" + reply.SerializeAsString();
            conn->send(response);
        }
    }
    else
    {
        cout << __FILE__ << " " << __LINE__ << endl;
    }
}

void Service::searchpick(const TcpConnectionPtr &conn, string s, Timestamp time)
{
    searchtickets_request st_;
    if (st_.ParseFromString(s))
    {
        int id = st_.id();
        int serial_num = st_.serial_num();

        if (check_serialnum(id, serial_num))
        {
            int st_ar_place = st_.st_ar_place();
            int st_place = st_ar_place / 10000;
            int ar_place = st_ar_place % 10000;
            int day = st_.day();
            string place_day = to_string(st_ar_place) + to_string(day);
            
            int port;
            if(st_ar_place/2){
                port = 6380;
            }
            else{
                port = 6381;
            }

            string sql = "lrange "+place_day+" 0 -1";
            vector<string> vs;
            Redis _redis;
            _redis.connect(port);


            int cids = _redis.get_Command_vs(vs,sql);

            searchtickets_reply reply;
            vector<string> msg;
            for(string s:vs){
                msg.push_back(s);
                _redis.get_Command_s(msg,"get "+ s);
            }

            int a = msg.size();
            for(int i = 0;i<a;i++){
                Pick_msg* pmsg = reply.add_tickets();
                //-------------------------
                pmsg->set_cid(stoi(msg[a]));    //danger!!!
                pmsg->set_s(stoi(msg[a+1] + msg[a+2] + msg[a+3] + msg[a+4] + msg[a+5]));
                pmsg->set_c(stoi(msg[a+6] + msg[a+7] + msg[a+8] + msg[a+9] + msg[a+10]));
                pmsg->set_st(stoi(msg[a+11]);
                a+=11;
            }

            string response = "16" + reply.SerializeAsString();
            conn->send(response);
        }
    }
    else
    {
        cout << __FILE__ << " " << __LINE__ << endl;
    }
}

void Service::bookpick_S(const TcpConnectionPtr &conn, string s, Timestamp time)
{
    bookticket_request bookpick_;
    if (bookpick_.ParseFromString(s))
    {
        int id = bookpick_.id();
        int serial_num = bookpick_.serial_num();

        if (check_serialnum(id, serial_num))
        {
            int cid = bookpick_.cid();
            int msg;
            string token_key = to_string(id) + to_string(cid);

            Redis redis_w;
            redis_w.connect(6379);
            int token = redis_w.get_Command_i("get " + token_key);
            if (token == -1)
            {
                int hope_col = bookpick_.hope_col();
                time tm = time(NULL);
                string cid_col = to_string(cid) + "S" + to_string(hope_col);
                string cid_col_lc = cid_col + "lc ";

                string sql = "set " + cid_col_lc + to_string(tm) + " EX 1 NX"; // one second period time

                // before write,should read first!!!

                while (redis_w.get_Command_i(sql))
                {
                }; // wait for lock (a bug)

                // run
                int pick = redis_w.get_Command_i("get " + cid_col) - 1;
                if (pick > 0)
                { // not last pick
                    redis_w.get_Command_i("set " + cid_col + " " + pick);
                    msg = 1; // temp msg_id
                }
                else if (pick == -2)
                {            // no pick
                    msg = 0; // temp msg_id
                }
                else
                { // last pick //if use MQ,can notice the other requests back
                    redis_w.get_Command_i("del " + cid_col);
                    msg = 1;
                }

                // unlock (change to use lua)
                if (redis_w.get_Command_i("get " + cid_col_lc) == tm)
                {
                    redis_w.get_Command_i("del " + cid_col_lc);
                }

                bookticket_reply reply;
                // then,write to mysql(later will use MQ)
                if (msg)
                {
                    char sql1[32] = {}; // search weather can choose site
                    char sql2[48] = {}; // book pick

                    MySQL mysql;
                    if (mysql.connect())
                    {
                        sprintf(sql1, "call prebookticket_S(%d)", cid);
                        int site_S, siteS_Cc;
                        MYSQL_RES *res = mysql.query(sql1);
                        if (res != nullptr)
                        {
                            MYSQL_ROW row = mysql_fetch_row(res);
                            if (row != nullptr)
                            {
                                site_S = stoi(row[0]);
                                siteS_Cc = stoi(row[1]);
                            }
                            else
                            {
                                cout << __FILE__ << " " << __LINE__ << endl;
                            }
                            mysql_free_result(res);
                        }

                        if (site_S == 0 && siteS_Cc == 0) // have no pick
                        {
                            msg = 0;
                        }
                        else
                        {
                            if (site_S == 0) // book ticket from Cancel_Ticket
                            {
                                int a = siteS_Cc / pow(10, hope_col - 1);
                                if (a % 10 == 0) // can choose col
                                {
                                    // choose the col which leave max site
                                    int b = 0, c = 0;
                                    hope_col = 0;
                                    while (siteS_Cc)
                                    {
                                        a = siteS_Cc % 10;
                                        c = max(a, b);
                                        if (b != c)
                                        {
                                            hope_col++;
                                            b = c;
                                            siteS_Cc /= 10;
                                        }
                                    }
                                }
                                sprintf(sql2, "call bookticket_SC(%d,%d,%d)", id, cid, hope_col);
                            }
                            else // book ticket from Ticket
                            {
                                int a = site_S / pow(10, hope_col - 1);
                                if (a % 10 == 0) // can choose col
                                {
                                    // choose the col which leave max site
                                    int b = 0, c = 0;
                                    hope_col = 0;
                                    while (site_S)
                                    {
                                        a = site_S % 10;
                                        c = max(a, b);
                                        if (b != c)
                                        {
                                            hope_col++;
                                            b = c;
                                            site_S /= 10;
                                        }
                                    }
                                }
                                sprintf(sql2, "call bookticket_SP(%d,%d,%d)", id, cid, hope_col);
                            }
                            res = mysql.query(sql2);
                            if (res != nullptr)
                            {
                                MYSQL_ROW row = mysql_fetch_row(res);
                                if (row != nullptr)
                                {
                                    msg = stoi(row[0]);
                                    mysql_free_result(res);
                                    redis_w.get_Command_i("set " + token_key + " " + to_string(msg));
                                }
                                else
                                {
                                    cout << __FILE__ << " " << __LINE__ << endl;
                                }
                            }
                        }
                    }
                }
            }
            else
            {
                msg = token;
            }

            reply.set_msg(msg);
            string response = "17" + reply.SerializeAsString();
            conn->send(response);
        }
    }
}

void Service::bookpick_C(const TcpConnectionPtr &conn, string s, Timestamp time)
{
    bookticket_request bookpick_;
    if (bookpick_.ParseFromString(s))
    {
        int id = bookpick_.id();
        int serial_num = bookpick_.serial_num();

        if (check_serialnum(id, serial_num))
        {
            int cid = bookpick_.cid();
            int msg;
            string token_key = to_string(id) + to_string(cid);

            Redis redis_w;
            redis_w.connect(6379);

            int token = redis_w.get_Command_i("get " + token_key);
            if (token == -1)
            {

                time tm = time(NULL);
                int hope_col = bookpick_.hope_col();

                string cid_col = to_string(cid) + "C" + to_string(hope_col);
                string cid_col_lc = cid_col + "lc ";

                string sql = "set " + cid_col_lc + to_string(tm) + " EX 1 NX"; // one second period time

                // before write,should read first!!!

                while (redis_w.get_Command_i(sql))
                {
                }; // wait for lock (a bug)

                // run
                int pick = redis_w.get_Command_i("get " + cid_col) - 1;
                if (pick > 0)
                { // not last pick
                    redis_w.get_Command_i("set " + cid_col + " " + pick);
                    msg = 1; // temp msg_id
                }
                else if (pick == -2)
                {            // no pick
                    msg = 0; // temp msg_id
                }
                else
                { // last pick //if use MQ,can notice the other requests back
                    redis_w.get_Command_i("del " + cid_col);
                    msg = 1;
                }

                // unlock (change to use lua)
                if (redis_w.get_Command_i("get " + cid_col_lc) == tm)
                {
                    redis_w.get_Command_i("del " + cid_col_lc);
                }

                bookticket_reply reply;

                if (msg)
                {
                    sprintf(sql1, "call prebookticket_C(%d)", cid);

                    MySQL mysql;
                    if (mysql.connect())
                    {
                        long site_C, siteC_Cc;
                        int msg;
                        MYSQL_RES *res = mysql.query(sql1);
                        if (res != nullptr)
                        {
                            MYSQL_ROW row = mysql_fetch_row(res);
                            if (row != nullptr)
                            {
                                site_C = stoi(row[0]);
                                siteC_Cc = stoi(row[1]);
                            }
                            else
                            {
                                cout << __FILE__ << " " << __LINE__ << endl;
                            }
                            mysql_free_result(res);
                        }

                        if (site_C == 0 && siteC_Cc == 0) // have no pick
                        {
                            msg = 0;
                        }
                        else
                        {
                            if (site_C == 0) // book ticket from Cancel_Ticket
                            {
                                int a = siteC_Cc / pow(100, hope_col - 1);
                                if (a % 100 == 0) // can choose col
                                {
                                    // choose the col which leave max site
                                    int b = 0, c = 0;
                                    hope_col = 0;
                                    while (siteC_Cc)
                                    {
                                        a = siteC_Cc % 100;
                                        c = max(a, b);
                                        if (b != c)
                                        {
                                            hope_col++;
                                            b = c;
                                            siteC_Cc /= 100;
                                        }
                                    }
                                }
                                sprintf(sql2, "call bookticket_CC(%d,%d,%d)", id, cid, hope_col);
                            }
                            else // book ticket from Ticket
                            {
                                int a = site_C / pow(1000, hope_col - 1);
                                if (a % 1000 == 0) // can choose col
                                {
                                    // choose the col which leave max site
                                    int b = 0, c = 0;
                                    hope_col = 0;
                                    while (site_C)
                                    {
                                        a = site_C % 1000;
                                        c = max(a, b);
                                        if (b != c)
                                        {
                                            hope_col++;
                                            b = c;
                                            site_C /= 1000;
                                        }
                                    }
                                }
                                sprintf(sql2, "call bookticket_CP(%d,%d,%d)", id, cid, hope_col);
                            }
                            res = mysql.query(sql2);
                            if (res != nullptr)
                            {
                                MYSQL_ROW row = mysql_fetch_row(res);
                                if (row != nullptr)
                                {
                                    msg = stoi(row[0]);
                                    mysql_free_result(res);
                                    redis_w.get_Command_i("set " + token_key + " " + to_string(msg));
                                }
                                else
                                {
                                    cout << __FILE__ << " " << __LINE__ << endl;
                                }
                            }
                        }
                    }
                }
            }
            else
            {
                msg = token;
            }
            reply.set_msg(msg);
            string response = "18" + reply.SerializeAsString();
            conn->send(response);
        }
    }
}

void Service::bookpick_ST(const TcpConnectionPtr &conn, string s, Timestamp time)
{
    bookticket_request bookpick_;
    if (bookpick_.ParseFromString(s))
    {
        int id = bookpick_.id();
        int serial_num = bookpick_.serial_num();

        if (check_serialnum(id, serial_num))
        {
            int cid = bookpick_.cid();
            int msg;
            string token_key = to_string(id) + to_string(cid);

            Redis redis_w;
            redis_w.connect(6379);

            int token = redis_w.get_Command_i("get " + token_key);
            if (token == -1)
            {
                time tm = time(NULL);

                string cid_col = to_string(cid) + "ST";
                string cid_col_lc = cid_col + "lc ";

                string sql = "set " + cid_col_lc + to_string(tm) + " EX 1 NX"; // one second period time

                // before write,should read first!!!

                while (redis_w.get_Command_i(sql))
                {
                }; // wait for lock (a bug)

                // run
                int pick = redis_w.get_Command_i("get " + cid_col) - 1;
                if (pick > 0)
                { // not last pick
                    redis_w.get_Command_i("set " + cid_col + " " + pick);
                    msg = 1; // temp msg_id
                }
                else if (pick == -2)
                {            // no pick
                    msg = 0; // temp msg_id
                }
                else
                { // last pick //if use MQ,can notice the other requests back
                    redis_w.get_Command_i("del " + cid_col);
                    msg = 1;
                }

                // unlock (change to use lua)
                if (redis_w.get_Command_i("get " + cid_col_lc) == tm)
                {
                    redis_w.get_Command_i("del " + cid_col_lc);
                }

                bookticket_reply reply;

                if (msg)
                {
                    char sql1[42] = {}; // search weather can choose site
                    sprintf(sql1, "call bookticket_ST(%d,%d)", id, cid);

                    bookticket_reply reply;

                    MySQL mysql;
                    if (mysql.connect())
                    {
                        MYSQL_RES *res = mysql.query(sql1);
                        if (res != nullptr)
                        {
                            MYSQL_ROW row = mysql_fetch_row(res);
                            if (row != nullptr)
                            {
                                msg = stoi(row[0]);
                                mysql_free_result(res);
                                redis_w.get_Command_i("set " + token_key + " " + to_string(msg));
                            }
                        }
                    }
                }
            }
            else
            {
                msg = token;
            }
            reply.set_msg(msg);
            string response = "19" + reply.SerializeAsString();
            conn->send(response);
        }
    }
    cout << __FILE__ << " " << __LINE__ << endl;
}

void Service::cancelbook(const TcpConnectionPtr &conn, string s, Timestamp time)
{
    cancelticket_request cct;

    if (cct.ParseFromString(s))
    {
        int id = cct.id();

        int serial_num = cct.serial_num();

        if (check_serialnum(id, serial_num))
        {
            int cid = cct.cid();

            int msg;
            string token_key = to_string(id) + to_string(cid);

            Redis redis_w;
            redis_w.connect(6379);

            int token = redis_w.get_Command_i("get " + token_key);
            if (token == -1)
            {
                char sql1[36];
                char sql2[44];

                cancelticket_reply reply;

                sprintf(sql1, "call precanceltickets(%d)", id);
                MySQL mysql;
                if (mysql.connect())
                {
                    MYSQL_RES *res = mysql.query(sql1);
                    if (res != nullptr)
                    {
                        MYSQL_ROW row = mysql_fetch_row(res);
                        if (stoi(row[0]) == 3)
                        {
                            reply.set_errno_id(700004);
                        }
                        else
                        {
                            mysql_free_result(res);

                            long site = cct.site();
                            int room = cct.room();
                            int col = site / 100000;

                            if (room == 0) // stand
                            {
                                sprintf(sql2, "call cancelticket_ST(%d,%d)", id, cid);
                            }
                            else if (room > 1) // common
                            {
                                sprintf(sql2, "call cancelticket_C(%d,%d)", id, cid);
                            }
                            else // first
                            {
                                sprintf(sql2, "call cancelticket_S(%d,%d)", id, cid);
                            }

                            res = mysql.query(sql2);
                            if (res != nullptr)
                            {
                                row = mysql_fetch_row(res);
                                if (!stoi(row[0]))
                                {
                                    reply.set_errno_id(700005);
                                    redis_w.get_Command_i("del " + token_key);
                                }
                                else
                                {
                                    reply.set_errno_id(700006);
                                }
                            }
                        }
                        mysql_free_result(res);
                        string response = "20" + reply.SerializeAsString();
                        conn->send(response);
                    }
                }
            }
        }
        else
        {
            cout << __FILE__ << " " << __LINE__ << endl;
        }
    }
    else
    {
        cout << __FILE__ << " " << __LINE__ << endl;
    }
}

void Service::personbook(const TcpConnectionPtr &conn, string s, Timestamp time)
{
    personbook_request pbr;
    if (pbr.ParseFromString(s))
    {
        int id = pbr.id();
        int serial_num = pbr.serial_num();

        if (check_serialnum(id, serial_num))
        {
            char sql[36] = {};
            sprintf(sql, "call personal_tickets(%d)", id);

            personbook_reply reply;

            MySQL mysql;
            if (mysql.connect())
            {
                MYSQL_RES *res = mysql.query(sql);
                if (res != nullptr)
                {
                    MYSQL_ROW row = mysql_fetch_row(res);

                    int st_ar_place = stoi(row[1]) * 10000 + stoi(row[2]);
                    while (row != nullptr)
                    {
                        Pick_msg2 *pm2 = reply.add_tickets();

                        pm2->set_cid(stoi(row[0]));
                        pm2->set_st_ar_place(st_ar_place);
                        pm2->set_day(stoi(row[3]));
                        pm2->set_st_ar_time(stoi(row[4]));
                        pm2->set_site(stoi(row[5]));
                        pm2->set_iscancel(stoi(row[6]));
                        pm2->set_price(stoi(row[7]));

                        row = mysql_fetch_row(res);
                    }
                    mysql_free_result(res);

                    reply.set_errno_id(false);
                    string response = "21" + reply.SerializeAsString();
                    conn->send(response);
                }
            }
        }
        else
        {
            cout << __FILE__ << " " << __LINE__ << endl;
        }
    }
    else
    {
        cout << __FILE__ << " " << __LINE__ << endl;
    }
}

void Service::getcitymodel(const TcpConnectionPtr &conn, string s, Timestamp time)
{
    citymodel_request citymodel_;
    if (citymodel_.ParseFromString(s))
    {
        int id = citymodel_.id();
        int serial_num = citymodel_.serial_num();

        if (check_serialnum(id, serial_num))
        {
            citymodel_reply reply;
            reply.set_msg(_redis.get_Command_s("get citymodel"));
            string response = "22" + reply.SerializeAsString();
            conn->send(response);
        }
    }
    else
    {
        cout << __FILE__ << " " << __LINE__ << endl;
    }
}

void Service::clientCloseException(const TcpConnectionPtr &conn)
{
    int id = 0;
    {
        lock_guard<mutex> lock(_mutex);
        for (auto it = connMap.begin(); it != connMap.end(); it++)
        {
            if (it->second == conn)
            {
                id = it->first;
                break;
            }
        }
        connMap.erase(id);
        slidewindowsMap.erase(id);
    }

    _redis.get_Command_i("del id" + id);
}

void Service::reset()
{
    for (auto it = connMap.begin(); it != connMap.end(); it++)
    {
        _redis.get_Command_i("del id" + to_string(it->first));
    }
}