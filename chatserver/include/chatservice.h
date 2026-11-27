#pragma once
#include<muduo/net/TcpServer.h>
#include<muduo/net/EventLoop.h>
#include<iostream>
#include "../../chatserver/thirdparty/json.hpp"
#include<functional>
#include <unordered_map>
#include "../../chatserver/include/server/usermodel.hpp"
#include<mutex>
#include "server/redis.hpp"
#include "../../chatserver/include/server/offlinemessagemodel.hpp"
#include "../../chatserver/include/server/friendmodel.hpp"
#include "../../chatserver/include/server/groupmodel.hpp"

using namespace std;
using namespace muduo;
using namespace muduo::net;
using json = nlohmann::json;
//消息处理器类型
using MsgHandler = std::function<void(const TcpConnectionPtr& conn, json& js, Timestamp)>;

//聊天服务器业务类
class ChatService
{
public:
	//单例模式，获取单例对象
	static ChatService* instance();
    bool start(EventLoop* loop);
	//重置方法，服务器异常时调用
	void reset();
	//处理登录业务
	void login(const TcpConnectionPtr& conn, json& js, Timestamp);
	//处理注册业务
	void reg(const TcpConnectionPtr& conn, json& js, Timestamp);
	//处理注销业务
	void loginout(const TcpConnectionPtr& conn, json& js, Timestamp);
	//一对一聊天业务
	void oneChat(const TcpConnectionPtr& conn, json& js, Timestamp);
	//添加好友业务
	void addFriend(const TcpConnectionPtr& conn, json& js, Timestamp);
	//创建群组业务
	void createGroup(const TcpConnectionPtr& conn, json& js, Timestamp);
	//加入群组业务
	void addGroup(const TcpConnectionPtr& conn, json& js, Timestamp);
	//群组聊天业务
	void groupChat(const TcpConnectionPtr & conn, json & js, Timestamp);

	//获取消息对应的处理器
	MsgHandler getHandler(int msgid);
	//处理客户端异常退出
	void clientCloseException(const TcpConnectionPtr& conn);

private:
	//私有构造函数，单例模式
	ChatService();
    void deliver(int userid, const string& message);
    void receiveRedis(int userid, const string& message);
    Redis _redis;
    EventLoop* _loop = nullptr;

	unordered_map<int, MsgHandler>_MsgHandlerMap; //存储消息id和对应的业务处理方法

	UserModel _useModel; //User数据操作类

	unordered_map<int, TcpConnectionPtr>_userConnMap; //存储在线用户的通信连接

	mutex _connMutex; //定义互斥锁，保护_userConnMap的线程安全

	OfflineMsgModel _offlineMsgModel; //离线消息数据操作类

	FriendModel _friendmodel; //好友数据操作类

	GroupModel _groupmodel; //群组数据操作类
};
