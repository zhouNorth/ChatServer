#pragma once
#include <string>
#include <vector>
#include "groupuser.hpp"
using namespace std;

//User表的ORM类，负责User表的操作
class Group
{
public:
	Group(int id = -1, string name = "", string desc = "")
	{	this->id = id;
		this->name = name;
		this->desc = desc;
	}
	void setId(int id) {
		this->id = id;
	}
	void setName(string name) {
		this->name = name;
	}
	void setDesc(string desc) {
		this->desc = desc;
	}

	int getId() {
		return this->id;
	}
	string getName() {
		return this->name;
	}
	string getDesc() {
		return this->desc;
	}
	vector<GroupUser>& getUsers() {
		return this->users;
	}
private:
	int id; // 群组id
	string name; // 群组名称
	string desc; // 群组描述
	vector<GroupUser> users; // 群组用户列表
};
