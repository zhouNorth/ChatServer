#include "../../include/chatservice.h"
#include "../../include/public.h"
#include<muduo/base/Logging.h>
using namespace muduo;
using namespace std::placeholders;
//单例模式，获取单例对象
ChatService* ChatService::instance() {
	static ChatService service;
	return &service;
}


//注册消息以及对应的Handler回调操作
ChatService::ChatService() {
	_MsgHandlerMap.insert({LOGIN_MSG, std::bind(&ChatService::login, this, _1, _2, _3)}	);
	_MsgHandlerMap.insert({ REG_MSG, std::bind(&ChatService::reg, this, _1, _2, _3) });
	_MsgHandlerMap.insert({ ONE_CHAT_MSG, std::bind(&ChatService::oneChat, this, _1, _2, _3) });
	_MsgHandlerMap.insert({ ADD_FRIEND_MSG, std::bind(&ChatService::addFriend, this, _1, _2, _3) });
	_MsgHandlerMap.insert({ CREATE_GROUP_MSG, std::bind(&ChatService::createGroup, this, _1, _2, _3) });
	_MsgHandlerMap.insert({ ADD_GROUP_MSG, std::bind(&ChatService::addGroup, this, _1, _2, _3) });
	_MsgHandlerMap.insert({ GROUP_CHAT_MSG, std::bind(&ChatService::groupChat, this, _1, _2, _3) });
	_MsgHandlerMap.insert({ LOGIN_OUT_MSG, std::bind(&ChatService::loginout, this, _1, _2, _3) });
}

bool ChatService::start(EventLoop* loop) {
    _loop = loop;
    return _redis.start([this](int userid, const string& message) {
        _loop->queueInLoop([this, userid, message] { receiveRedis(userid, message); });
    });
}

void ChatService::clientCloseException(const TcpConnectionPtr& conn) {
    int userid = -1;
    {
        lock_guard<mutex> lock(_connMutex);
        for (auto it = _userConnMap.begin(); it != _userConnMap.end(); ++it) {
            if (it->second == conn) {
                userid = it->first;
                _userConnMap.erase(it);
                break;
            }
        }
    }
    if (userid == -1) return;
    _redis.unsubscribe(userid);
    User user;
    user.setId(userid);
    user.setState("offline");
    _useModel.updateState(user);
}

void ChatService::reset() {
    vector<int> users;
    {
        lock_guard<mutex> lock(_connMutex);
        for (const auto& entry : _userConnMap) users.push_back(entry.first);
        _userConnMap.clear();
    }
    _redis.stop();
    for (int id : users) {
        User user;
        user.setId(id);
        user.setState("offline");
        _useModel.updateState(user);
    }
}

void ChatService::receiveRedis(int userid, const string& message) {
    TcpConnectionPtr target;
    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(userid);
        if (it != _userConnMap.end()) target = it->second;
    }
    if (target && target->connected()) target->send(message + "\n");
    else _offlineMsgModel.insert(userid, message);
}

void ChatService::deliver(int userid, const string& message) {
    TcpConnectionPtr target;
    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(userid);
        if (it != _userConnMap.end()) target = it->second;
    }
    if (target && target->connected()) {
        target->send(message + "\n");
        return;
    }
    if (_redis.publish(userid, message) <= 0) _offlineMsgModel.insert(userid, message);
}

void ChatService::login(const TcpConnectionPtr& conn, json& js, Timestamp) {
	int id = js["id"].get<int>();	//用户id
	string pwd = js["password"];	//用户密码
	User user = _useModel.query(id);
	if (id==user.getId()&&pwd == user.getPassword()) {
		if (!_useModel.claimOnline(id)) {
			//用户已经登录
			json response;
			response["msgid"] = LOGIN_MSG_ACK;
			response["error"] = 2;
			response["errmsg"] = "user is already online";
			conn->send(response.dump() + "\n");
		}
		else {
			//登录成功

			//记录用户连接信息
			{
				lock_guard<mutex> lock(_connMutex);
				_userConnMap.insert({ id, conn });
			}

			//更新在线状态
            if (!_redis.subscribe(id)) {
                clientCloseException(conn);
                json failed = {{"msgid", LOGIN_MSG_ACK}, {"error", 3},
                               {"errmsg", "Redis subscription unavailable; retry login"}};
                conn->send(failed.dump() + "\n");
                return;
            }
			json response;
			response["msgid"] = LOGIN_MSG_ACK;
			response["error"] = 0;
			response["id"] = user.getId();
			response["name"] = user.getName();

			//查询是否有离线消息
			vector<string>vec = _offlineMsgModel.query(id);
			if (!vec.empty())
			{
				response["offlinemsg"] = vec;
				_offlineMsgModel.remove(id); //删除离线消息
			}

			//查询好友信息并返回
			vector<User>userVec = _friendmodel.query(id);
			if (!userVec.empty()) {
				vector<string>vec2;
				for (User &f:userVec) {
					json friend_js;
					friend_js["id"] = f.getId();
					friend_js["name"] = f.getName();
					friend_js["state"] = f.getState();
					vec2.push_back(friend_js.dump());
				}response["friends"] = vec2;
			}

			//查询群组信息并返回
			vector<Group>groupVec = _groupmodel.queryGroups(id);
			 if (!groupVec.empty()) {
				 vector<string>vec3;
				 for (Group &group:groupVec) {
					 json groupjs;
					 groupjs["id"] = group.getId();
					 groupjs["name"] = group.getName();
					 groupjs["desc"] = group.getDesc();
					 vector<string>userVec;
					 for (GroupUser &user:group.getUsers()) {
						 json userjs;
						 userjs["id"] = user.getId();
						 userjs["name"] = _useModel.query(user.getId()).getName();
						 userjs["role"] = user.getRole();
						 userVec.push_back(userjs.dump());
					 }
					 groupjs["users"] = userVec;
					 vec3.push_back(groupjs.dump());
				 }response["groups"] = vec3;
			 }


			conn->send(response.dump() + "\n");
		}
	}
	else {
		//登录失败
		json response;
		response["msgid"] = LOGIN_MSG_ACK;
		response["error"] = 1;
		response["errmsg"] = "secret erro";
		conn->send(response.dump() + "\n");

	}

}

//处理注册业务
void ChatService::reg(const TcpConnectionPtr& conn, json& js, Timestamp) {
	string name = js["name"];
	string pwd = js["password"];
	User user;
	user.setName(name);
	user.setPassword(pwd);
	bool state = _useModel.insert(user);
	if (state) {
		json response;
		response["msgid"] = REG_MSG_ACK;
		response["error"] = 0;
		response["id"] = user.getId();
		conn->send(response.dump() + "\n");
	}//注册成功
	else {
		json response;
		response["msgid"] = REG_MSG_ACK;
		response["error"] = 1;
		response["id"] = user.getId();
		conn->send(response.dump() + "\n");
		//注册失败
	}
}

void ChatService::loginout(const TcpConnectionPtr& conn, json&, Timestamp) {
    clientCloseException(conn);
}

MsgHandler ChatService::getHandler(int msgid) {
    auto handler = _MsgHandlerMap.find(msgid);
    if (handler == _MsgHandlerMap.end()) {
        return [](const TcpConnectionPtr& conn, json&, Timestamp) {
            conn->send("{\"msgid\":11,\"error\":1,\"errmsg\":\"unknown message type\"}\n");
        };
    }
    MsgHandler action = handler->second;
    return [this, msgid, action](const TcpConnectionPtr& conn, json& js, Timestamp time) {
        int userid = -1;
        {
            lock_guard<mutex> lock(_connMutex);
            for (const auto& entry : _userConnMap) if (entry.second == conn) {
                userid = entry.first;
                break;
            }
        }
        if (msgid == LOGIN_MSG || msgid == REG_MSG) {
            if (userid != -1) {
                conn->send("{\"msgid\":11,\"error\":1,\"errmsg\":\"already authenticated\"}\n");
                return;
            }
        } else {
            if (userid == -1) {
                conn->send("{\"msgid\":11,\"error\":1,\"errmsg\":\"login required\"}\n");
                return;
            }
            js["id"] = userid;
        }
        action(conn, js, time);
    };
}

void ChatService::oneChat(const TcpConnectionPtr&, json& js, Timestamp) {
    deliver(js["toid"].get<int>(), js.dump());
}

void ChatService::addFriend(const TcpConnectionPtr& conn, json& js, Timestamp)
{
	int userid = js["id"].get<int>();
	int friendid = js["friendid"].get<int>();

	//存储好友消息
	_friendmodel.insert(userid,friendid);
}

//创造群组业务
void ChatService::createGroup(const TcpConnectionPtr& conn, json& js, Timestamp)
{
	int userid = js["id"].get<int>();
	string name = js["groupname"];
	string desc = js["groupdesc"];
	Group group(-1,name,desc);
	if (_groupmodel.createGroup(group)) {
		//存储群组信息
		_groupmodel.addGroup(userid,group.getId(),"creator");

	}
}

//加入群组业务
void ChatService::addGroup(const TcpConnectionPtr& conn, json& js, Timestamp)
{
	int userid = js["id"].get<int>();
	int groupid = js["groupid"];

	_groupmodel.addGroup(userid,groupid,"normal");
}

//群组聊天业务
void ChatService::groupChat(const TcpConnectionPtr&, json& js, Timestamp) {
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    vector<int> recipients = _groupmodel.queryGroupUsers(userid, groupid);
    for (int id : recipients) deliver(id, js.dump());
}
