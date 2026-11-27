#ifndef DB_H
#define DB_H

#include <mysql/mysql.h>
#include <string>
#include <cstdlib>
#include <iostream>
#include <muduo/base/Logging.h>
using namespace std;

// 数据库配置信息
inline string dbEnv(const char* key, const char* fallback) {
    const char* value = std::getenv(key);
    return value ? value : fallback;
}
static string server = dbEnv("CHAT_DB_HOST", "127.0.0.1");
static string user = dbEnv("CHAT_DB_USER", "root");
static string password = dbEnv("CHAT_DB_PASSWORD", "123456");
static string dbname = dbEnv("CHAT_DB_NAME", "chat");

// 数据库操作类
class MySQL
{
public:
    // 初始化数据库连接
    MySQL();
    // 释放数据库连接资源
    ~MySQL();

    // 连接数据库
    bool connect();
    // 执行SQL语句（更新操作：insert, update, delete）
    bool update(const string& sql);

    // 查询操作
    MYSQL_RES* query(const string& sql);

    // 获取连接
    MYSQL* getConnection();

private:
    MYSQL* _conn;
};

#endif
