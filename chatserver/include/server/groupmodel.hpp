#pragma once

#include "group.hpp"
#include "groupuser.hpp"
#include <string>
#include<vector>
using namespace std;
class GroupModel
{
public:
	//创建群组
	bool createGroup(Group &group);
	//加入群组
	void addGroup(int userid,int groupid,string role);
	//查询用户所在的群组信息
	vector<Group> queryGroups(int userid);
	//根据groupid查询群组用户id列表，除了管理员和群主之外的普通成员
	vector<int> queryGroupUsers(int userid,int groupid);
};
