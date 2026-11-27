#include "../../include/server/usermodel.hpp"
#include "../../include/server/db/db.h"
#include <iostream>
using namespace std;
//User表的增加方法
bool UserModel::insert(User& user) {
	//1.组装sql语句
	char sql[1024] = { 0 };
	sprintf(sql, "insert into user(name,password,state)values('%s','%s','%s')",
		user.getName().c_str(), user.getPassword().c_str(), user.getState().c_str());

	MySQL mysql;
	//2.连接数据库
	if (mysql.connect()) {
		LOG_INFO << "connect success ";
		if (mysql.update(sql)) {
			//获取插入成功的主键id
			LOG_INFO << "insert success ";
			user.setId(mysql_insert_id(mysql.getConnection()));
			return true;
		}

	}LOG_INFO << "insert false " ;
	return false;
};
User UserModel::query(int id) {
	//1.组装sql语句
	char sql[1024] = { 0 };
	sprintf(sql, "select * from user where id=%d", id);

	//2.连接数据库
	MySQL mysql;
	if (mysql.connect()) {
		LOG_INFO << "connect success ";
		if (MYSQL_RES* res = mysql.query(sql)) {
			LOG_INFO << "query success ";
			MYSQL_ROW row = mysql_fetch_row(res);
			if (row != nullptr) {
				User user;
				user.setId(atoi(row[0]));
				user.setName(row[1]);
				user.setPassword(row[2]);
				user.setState(row[3]);
				mysql_free_result(res);
				return user;
			}
			mysql_free_result(res);
			LOG_INFO << "no data";
		}
		else {
		LOG_INFO << "query failed";
		return User();
		}
	}
	return User();
}
bool UserModel::updateState(User user)
{
	//1.组装sql语句
	char sql[1024] = { 0 };
	sprintf(sql, "update user set state = '%s' where id = %d", user.getState().c_str(), user.getId());

	MySQL mysql;
	//2.连接数据库
	if (mysql.connect()) {

		if (mysql.update(sql)) {
			LOG_INFO << "update success ";
			return true;
		}

	}
	return false;
}

//重置用户状态
void UserModel::resetState() {
	//1.组装sql语句
	char sql[1024] = "update user set state = 'offline' ";


	MySQL mysql;
	//2.连接数据库
	if (mysql.connect()) {

		mysql.update(sql);

	}

}


bool UserModel::claimOnline(int id) {
    MySQL mysql;
    if (!mysql.connect()) return false;
    string sql = "update user set state='online' where id=" + to_string(id) + " and state='offline'";
    return mysql.update(sql) && mysql_affected_rows(mysql.getConnection()) == 1;
}
