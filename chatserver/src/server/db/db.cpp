#include "../../../../chatserver/include/server/db/db.h"
// 初始化数据库连接
MySQL::MySQL()
{
    _conn = mysql_init(nullptr);
}

// 释放数据库连接资源
MySQL::~MySQL()
{
    if (_conn != nullptr)
        mysql_close(_conn);
}

// 连接数据库
 bool MySQL::connect()
{
    MYSQL* p = mysql_real_connect(_conn, server.c_str(), user.c_str(),
        password.c_str(), dbname.c_str(), static_cast<unsigned int>(std::stoul(dbEnv("CHAT_DB_PORT", "3306"))), nullptr, 0);
    if (p != nullptr) {
		// 设置字符集
		mysql_set_character_set(_conn, "utf8mb4");
        LOG_INFO<< "connect mysql success!";
    }
    else {
		LOG_INFO << "connect mysql failed!";
        LOG_INFO << "尝试连接 MySQL → host:" << server
            << " user:" << user
            << " db:" << dbname
            << " port:3306";
    }
    return p ;
}

// 执行SQL语句（更新操作：insert, update, delete）
 bool MySQL::update(const string& sql)
 {
     if (mysql_query(_conn, sql.c_str()))
     {
         LOG_INFO << __FILE__ << ":" << __LINE__ << " SQL执行失败: " << sql
             << "，错误: " << mysql_error(_conn);
         return false;
     }
     return true;
 }
// 查询操作
MYSQL_RES* MySQL::query(const string& sql)
{
    if (mysql_query(_conn, sql.c_str()))
    {
        LOG_INFO << __FILE__ << ":" << __LINE__ << ":"
            << sql << "查询失败!";
        return nullptr;
    }
    return mysql_use_result(_conn);
}

// 获取连接
MYSQL* MySQL::getConnection()
{
    return _conn;
}
