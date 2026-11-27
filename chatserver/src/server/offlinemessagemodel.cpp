#include "../../include/server/offlinemessagemodel.hpp"
#include "../../include/server/db/db.h"
#include <iostream>
using namespace std;


//存储用户离线消息
void OfflineMsgModel::insert(int userid, string msg) {
    MySQL mysql;
    if (!mysql.connect()) return;
    string escaped(msg.size() * 2 + 1, '\0');
    auto size = mysql_real_escape_string(mysql.getConnection(), &escaped[0], msg.data(), msg.size());
    escaped.resize(size);
    mysql.update("insert into offlinemsg values(" + to_string(userid) + ",'" + escaped + "')");
}

void OfflineMsgModel::remove(int userid) {
    //1.组装sql语句
    char sql[1024] = { 0 };
    sprintf(sql, "delete from offlinemsg where userid = %d", userid);

    MySQL mysql;
    //2.连接数据库
    if (mysql.connect()) {

        if (mysql.update(sql)) {
            LOG_INFO << "delete offlinemsg success ";

        }

    }
}
    // 查询用户离线消息
    vector<string> OfflineMsgModel::query(int userid) {
        // 1. 组装SQL语句
        char sql[1024] = { 0 };
        sprintf(sql, "select message from offlinemsg where userid=%d", userid);

        // 2. 连接数据库
        MySQL mysql;
        vector<string> vec;
        if (mysql.connect()) {
            LOG_INFO << "connect success ";
            MYSQL_RES* res = mysql.query(sql);
            if (res != nullptr) {
                MYSQL_ROW row;
                while ((row = mysql_fetch_row(res)) != nullptr) {
                    vec.push_back(row[0]);
                }
                mysql_free_result(res);
            }
            // 无论是否有结果，都返回 vec（可能为空）
        }
        return vec;   // 连接失败也返回空vector
    }
