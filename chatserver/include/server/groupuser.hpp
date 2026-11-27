#pragma once
#include "user.hpp"

//群组用户，多了一个群组角色信息
class GroupUser : public User
{
public:
	void setRole(string role) {
		this->role = role;
	}
	string getRole() {
		return this->role;
	}
private:
	string role; // 群组角色：owner-群主，admin-管理员，normal-普通成员
};
