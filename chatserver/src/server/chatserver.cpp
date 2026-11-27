#include "../../include/chatserver.h"
#include<functional>
#include "../../thirdparty/json.hpp"
#include<string>
#include<algorithm>
#include "../../include/chatservice.h"
using namespace placeholders;
using json = nlohmann::json;
using namespace std;

ChatServer::ChatServer(EventLoop* loop,
	const InetAddress& listenAddr,
	const string& nameArg): _server(loop,listenAddr,nameArg),_loop(loop) {
	//注册连接回调函数
	_server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));
	//注册消息回调函数
	_server.setMessageCallback(std::bind(&ChatServer::onMessage,this,_1,_2,_3));
}

//启动服务
void ChatServer::start() {
	_server.start();
}

//处理连接与断开
void ChatServer::onConnection(const TcpConnectionPtr& conn) {
	if (!conn->connected()) {
		LOG_INFO << "offline success";
		ChatService::instance()->clientCloseException(conn);
		conn->shutdown(); //关闭连接
	}
}

//处理读写事件
void ChatServer::onMessage(const TcpConnectionPtr& conn, Buffer* buffer, Timestamp time) {
    constexpr size_t limit = 1024 * 1024;
    while (buffer->readableBytes()) {
        const char* begin = buffer->peek();
        const char* end = begin + buffer->readableBytes();
        const char* delimiter = std::find_if(begin, end, [](char c) { return c == '\n' || c == '\0'; });
        if (static_cast<size_t>(delimiter - begin) > limit) {
            buffer->retrieveAll();
            conn->shutdown();
            return;
        }
        if (delimiter == end) return;
        std::string frame(begin, delimiter);
        buffer->retrieve(static_cast<size_t>(delimiter - begin) + 1);
        if (frame.empty()) continue;
        try {
            json js = json::parse(frame);
            auto handler = ChatService::instance()->getHandler(js.at("msgid").get<int>());
            handler(conn, js, time);
        } catch (const std::exception& error) {
            LOG_ERROR << "Invalid request: " << error.what();
            conn->send("{\"msgid\":11,\"error\":1,\"errmsg\":\"invalid request\"}\n");
        }
    }
}
