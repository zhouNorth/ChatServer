#pragma once
#include "db/db.h"
#include "user.hpp"
//User表的数据操作类
class UserModel {
public:
	bool insert(User& user); //插入数据

	User query(int id); //根据id查询用户信息

	bool claimOnline(int id);
	bool updateState(User user); //更新用户状态

	void resetState(); //重置用户状态
};
