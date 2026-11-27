//#include <cstdio>
//#include<muduo/net/TcpServer.h>
//#include<muduo/net/EventLoop.h>
//#include<functional>
//#include<iostream>
//#include<string>
//#include "../thirdparty/json.hpp"
//using namespace std;
//using namespace muduo;
//using namespace muduo::net;
//using namespace placeholders;
//using json = nlohmann::json;
//
///*基于muduo网络库开发服务器程序
//1.组合TcpServer对象
//2.创建EventLoop事件循环对象的指针
//3.明确TcpServer构造函数需要哪些参数，输出ChatServer的构造函数
//4.注册设置处理连接的回调函数和处理函数读写的回调函数
//5.设置合适的服务端线程数量，muduo会自动划分I/O线程和work线程
//*/
//class ChatServer {
//public:
//    ChatServer(EventLoop* loop,//事件循环
//        const InetAddress& listenAddr,//ip和端口
//        const string& nameArg) //名字
//        :_server(loop,listenAddr,nameArg),_loop(loop)
//    {
//        //给服务器注册连接和断开的回调
//        _server.setConnectionCallback(std::bind(&ChatServer::onConnection,this,_1));
//
//        //给服务器注册用户读写事件回调
//        _server.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));
//
//        //划分线程
//        _server.setThreadNum(4);
//    }
//
//    //开启事件循环
//    void start() {
//        _server.start();
//    }
//private:
//    //专门用于处理连接与断开
//    void onConnection(const TcpConnectionPtr& conn) {
//        if (conn->connected()) {
//            cout << conn->peerAddress().toIpPort()<<"->"<<conn->localAddress().toIpPort()<<"stata:online" << endl;
//        }
//        else {
//            cout << conn->peerAddress().toIpPort() << "->" << conn->localAddress().toIpPort() <<"stata:offline" << endl;
//			conn->shutdown();
//			//_loop->quit();
//        }
//
//    }
//    //专门用于处理读写事件
//    void onMessage(const TcpConnectionPtr& conn, //连接
//                   Buffer *buffer,                   //缓冲区
//                   Timestamp time)      //接收到数据的时间
//    {
//        string buf = buffer->retrieveAllAsString();
//        cout << "recv data:" << buf << "time:" << time.toString() << endl;
//        conn->send(buf);
//    }
//
//    TcpServer _server;
//    EventLoop* _loop; //epoll
//};
//
//int main()
//{
//    cout << "main start" << endl;
//	EventLoop loop;
//    InetAddress listenAddr("127.0.0.1",6000);
//	ChatServer server(&loop, listenAddr, "ChatServer");
//	server.start();
//	loop.loop();
//
//    return 0;
//}
