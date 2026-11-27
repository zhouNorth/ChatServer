#include "../../include/server/friendmodel.hpp"

void FriendModel::insert(int userid, int friendid)
{
	//1.组装sql语句
	char sql[1024] = { 0 };
	sprintf(sql, "insert into friend values(%d,%d)", userid, friendid);

	MySQL mysql;
	//2.连接数据库
	if (mysql.connect()) {

		if (mysql.update(sql)) {
			LOG_INFO << "insert friend success ";
		}
	}
}
vector<User>FriendModel::query(int userid)
{
    // 1. 组装SQL语句
    char sql[1024] = { 0 };
    sprintf(sql, "select a.id,a.name,a.state from user a inner join friend b on b.friendid = a.id where b.userid=%d", userid);

    // 2. 连接数据库
    MySQL mysql;
    vector<User> vec;
    if (mysql.connect()) {
        LOG_INFO << "connect success ";
        MYSQL_RES* res = mysql.query(sql);
        if (res != nullptr) {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr) {
                User user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                vec.push_back(user);
            }
            mysql_free_result(res);
        }
        // 无论是否有结果，都返回 vec（可能为空）
    }
    return vec;   // 连接失败也返回空vector
}
