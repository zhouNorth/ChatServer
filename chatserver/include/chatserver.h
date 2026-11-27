#pragma once
#include<muduo/net/TcpServer.h>
#include<muduo/net/EventLoop.h>
#include<iostream>
#include "../../chatserver/thirdparty/json.hpp"


using namespace std;
using namespace muduo;
using namespace muduo::net;
using json = nlohmann::json;
//聊天服务器主类
class ChatServer
{
public:
	//构造函数，初始化服务器对象
	ChatServer(EventLoop* loop,
		const InetAddress& listenAddr,
		const string& nameArg);
	//启动服务
	void start();
private:
	void onConnection(const TcpConnectionPtr& conn); //专门用于处理连接与断开
	void onMessage(const TcpConnectionPtr& conn,	//专门用于处理读写事件
		Buffer* buffer,
		Timestamp time);
	TcpServer _server; //组合TcpServer对象
	EventLoop* _loop; //创建EventLoop事件循环对象的指针
};
