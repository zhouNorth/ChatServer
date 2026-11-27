#include "../../include/server/groupmodel.hpp"
#include "../../include/server/db/db.h"
#include <iostream>
using namespace std;

bool GroupModel::createGroup(Group& group)
{
    //1.组装sql语句
    char sql[1024] = { 0 };
    sprintf(sql, "insert into all_group (groupname,groupdesc) values('%s','%s')", group.getName().c_str(), group.getDesc().c_str());

    MySQL mysql;
    //2.连接数据库
    if (mysql.connect()) {

        if (mysql.update(sql)) {
			group.setId(mysql_insert_id(mysql.getConnection())) ;
            return true;

        }

    }
    return false;
}

void GroupModel::addGroup(int userid, int groupid, string role)
{
    //1.组装sql语句
    char sql[1024] = { 0 };
    sprintf(sql, "insert into groupuser values(%d,%d,'%s')", userid, groupid, role.c_str());

    MySQL mysql;
    //2.连接数据库
    if (mysql.connect()) {

        if (mysql.update(sql)) {

			LOG_INFO << "add groupuser success ";
        }

    }

}

vector<Group> GroupModel::queryGroups(int userid)
{
    // 1. 组装SQL语句
    char sql[1024] = { 0 };
    sprintf(sql, "select a.id,a.groupname,a.groupdesc from all_group a inner join groupuser b on b.groupid = a.id where b.userid=%d", userid);
    // 2. 连接数据库
    MySQL mysql;
    vector<Group> vec;
    if (mysql.connect()) {
        LOG_INFO << "connect success ";
        MYSQL_RES* res = mysql.query(sql);
        if (res != nullptr) {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr) {
                Group group;
                group.setId(atoi(row[0]));
                group.setName(row[1]);
                group.setDesc(row[2]);
                vec.push_back(group);
            }
            mysql_free_result(res);
        }
        // 无论是否有结果，都返回 vec（可能为空）
    }


    for (Group& group : vec)
	{   // 3. 查询群组用户信息
        sprintf(sql, "select a.id,a.name,a.state,b.grouprole from user a inner join groupuser b on b.userid = a.id where b.groupid=%d", group.getId());
        MYSQL_RES* res = mysql.query(sql);
        if (res != nullptr) {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr) {
               GroupUser user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                user.setRole(row[3]);
				group.getUsers().push_back(user);
            }
            mysql_free_result(res);
        }
    }

     return vec;   // 连接失败也返回空vector
}

vector<int> GroupModel::queryGroupUsers(int userid,int groupid)
{
    // 1. 组装SQL语句
    char sql[1024] = { 0 };
    sprintf(sql, "select userid from groupuser where groupid=%d and userid!=%d", groupid,userid);
    // 2. 连接数据库
    MySQL mysql;
    vector<int> vec;
    if (mysql.connect()) {
        LOG_INFO << "connect success ";
        MYSQL_RES* res = mysql.query(sql);
        if (res != nullptr) {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr) {
                vec.push_back(atoi(row[0]));
            }
            mysql_free_result(res);
        }
        // 无论是否有结果，都返回 vec（可能为空）
    }
	return vec;   // 连接失败也返回空vector
}
