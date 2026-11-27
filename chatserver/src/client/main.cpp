#include "../../thirdparty/json.hpp"
#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include <chrono>
#include <ctime>
#include <cstring>
#include <cstdlib>
#include <functional>
#include <unordered_map>
#include <sys/socket.h>
using namespace std;
using json = nlohmann::json;

// The main thread receives login replies, then hands receive ownership to readTaskHandler.
static string receiveBuffer;
static string receiveFrame(int fd) {
    for (;;) {
        auto end = receiveBuffer.find('\n');
        if (end != string::npos) {
            if (end > 1024 * 1024) return {};
            string frame = receiveBuffer.substr(0, end);
            receiveBuffer.erase(0, end + 1);
            return frame;
        }
        if (receiveBuffer.size() > 1024 * 1024) return {};
        char chunk[4096];
        auto count = recv(fd, chunk, sizeof(chunk), 0);
        if (count <= 0) return {};
        receiveBuffer.append(chunk, static_cast<size_t>(count));
    }
}


#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "../../include/server/group.hpp"
#include "../../include/server/user.hpp"
#include "../../include/public.h"

//记录当前系统登录的用户信息
User g_currentUser;
//记录当前系统登录的用户好友列表信息
vector<User> g_currentUserFriendList;
//记录当前系统登录的用户群组列表信息
vector<Group> g_currentUserGroupList;
//显示当前登录成功用户的基本信息
void showCurrentUserData();

//接收线程
void readTaskHandler(int clientfd);
//获取系统时间（聊天信息需要添加时间戳）
string getCurrentTime();
//主界面程序
void mainMenu(int clientfd);
//帮助信息
void help(int fd = 0 , string str="");
//聊天消息
void chat(int fd, string str);
//添加好友
void addFriend(int fd, string str);
//创建群组
void createGroup(int fd, string str);
//加入群组
void addGroup(int fd, string str);
//群组聊天
void groupchat(int fd, string str);
//退出系统
void loginout(int fd, string str);
//记录当前是否正在显示主菜单
bool isMenuRunning = false;

//聊天客户端程序实现，main线程用作发送线程，readTaskHandler线程用作接收线程
int main(int argc, char** argv)
{
	if (argc < 3)
	{
		cerr << "command invalid! example : ./ChatClient 127.0.0.1 6000" << endl;
		exit(-1);
	}
	//解析命令行参数，获取服务器的ip和端口号
	char* ip = argv[1];
	uint16_t port = atoi(argv[2]);

	//创建client端的socket
	int clientfd = socket(AF_INET, SOCK_STREAM, 0);
	if (clientfd == -1)
	{
		cerr << "create socket error" << endl;
		exit(-1);
	}

	//绑定服务器的ip和端口号
	sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	server_addr.sin_addr.s_addr = inet_addr(ip);

	//连接服务器
	if (-1 == connect(clientfd, (sockaddr*)&server_addr, sizeof(server_addr)))
	{
		cerr << "connect error" << endl;
		close(clientfd);
		exit(-1);
	}

	for (;;)
	{
		//显示首页菜单 登录、注册、退出
		cout << "========================" << endl;
		cout << "1. login" << endl;
		cout << "2. register" << endl;
		cout << "3. quit" << endl;
		cout << "========================" << endl;
		cout << "choice: ";
		int choice = 0;
		cin >> choice;
		cin.get(); // 处理输入缓冲区中的换行符
		switch (choice)
		{
		case 1:
		{
			int id = 0;
			char password[50] = { 0 };
			cout << "user id: ";
			cin >> id;
			cin.get(); // 处理输入缓冲区中的换行符
			cout << "user password: ";
			cin.getline(password, 50);

			//封装登录json数据
			json js;
			js["msgid"] = LOGIN_MSG;
			js["id"] = id;
			js["password"] = password;

			//发送登录数据
			string request = js.dump();
			int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
			if (-1 == len)
			{
				cerr << "send login msg error!" << endl;
			}
			else
			{
				string buffer = receiveFrame(clientfd);
                len = buffer.empty() ? -1 : static_cast<int>(buffer.size());
				if (-1 == len)
				{
					cerr << "recv login response error!" << endl;
					break;

				}
				else
				{
					json response = json::parse(buffer);
					if (response["error"].get<int>() == 0)
					{
						//登录成功，记录当前用户的id和name
						g_currentUser.setId(id);
						g_currentUser.setName(response["name"].get<string>());

						//记录当前用户的好友列表信息
						if (response.contains("friends"))
						{
							vector<string> friends = response["friends"];
							for (auto& str : friends)
							{
								json js = json::parse(str);
								User user;
								user.setId(js["id"].get<int>());
								user.setName(js["name"].get<string>());
								user.setState(js["state"].get<string>());
								g_currentUserFriendList.push_back(user);
							}
						}

						//记录当前用户的群组列表信息
						if (response.contains("groups"))
						{
							vector<string>groups = response["groups"];
							for (auto& str : groups)
							{
								json js = json::parse(str);
								Group group;
								group.setId(js["id"].get<int>());
								group.setName(js["name"]);
								group.setDesc(js["desc"]);

								vector<string>users = js["users"];
								for (auto& userstr : users)
								{
									json userjs = json::parse(userstr);
									GroupUser user;
									user.setId(userjs["id"].get<int>());
									user.setName(userjs["name"].get<string>());
									user.setRole(userjs["role"].get<string>());
									group.getUsers().push_back(user);
								}
								g_currentUserGroupList.push_back(group);
							}

						}
						//显示登录成功用户的基本信息
						showCurrentUserData();

						//显示当前用户的离线消息
						if (response.contains("offlinemsg"))
						{
							vector<string> offlinemsgs = response["offlinemsg"];
							for (auto& str : offlinemsgs)
							{
								json js = json::parse(str);
								cout << js["time"] << "[" << js["id"] << "]:" << js["name"] << "said" << js["msg"] << endl;
							}
						}

						//登录成功，启动接收线程，接收服务器转发的消息
						int static readNum = 0;
						if (readNum == 0)
						{
							readNum++;
							std::thread readTask(readTaskHandler, clientfd);
							readTask.detach();
						}


						isMenuRunning = true;
						//进入主界面，显示主菜单
						mainMenu(clientfd);
					}
					else if (response["error"].get<int>() == 1)
					{
						//登录失败，显示错误信息
						cerr << "login failed, error message: " << response["errmsg"] << endl;
					}
					else
					{
						//用户已经在线，显示错误信息
						cerr << "login failed, error message: " << response["errmsg"] << endl;
					}
				}

			}
		}
		break;
		case 2: //注册业务
		{
			char name[50] = { 0 };
			char password[50] = { 0 };
			cout << "user name: ";
			cin.getline(name, 50);
			cout << "user password: ";
			cin.getline(password, 50);

			//封装注册json数据
			json js;
			js["msgid"] = REG_MSG;
			js["name"] = name;
			js["password"] = password;
			string request = js.dump();
			int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
			if (-1 == len)
			{
				cerr << "send reg msg error!" << endl;
				break;
			}
			else
			{
				string buffer = receiveFrame(clientfd);
                len = buffer.empty() ? -1 : static_cast<int>(buffer.size());
				if (-1 == len)
				{
					cerr << "recv reg response error!" << endl;
					break;
				}
				else
				{
					json response = json::parse(buffer);
					if (response["error"].get<int>() == 0)
					{
						cout << "reg success, user id is " << response["id"] << endl;
					}
					else
					{
						cout << "reg failed, user id is " << response["id"] << endl;
					}
				}
			}
		}
		break;
		case 3: //退出业务
		{
			close(clientfd);
			cout << "bye" << endl;
			exit(0);
		}break;
		default:
			cout << "invalid input!" << endl;
			break;
		}
	}
}
//显示当前登录成功用户的基本信息
void showCurrentUserData()
{
	cout << "===========user introduction=============" << endl;
	cout << "user id: " << g_currentUser.getId() << endl;
	cout << "user name: " << g_currentUser.getName() << endl;
	cout << "===========friends============= " << endl;
	for (User& user : g_currentUserFriendList)
	{
		cout << user.getId() << " " << user.getName() << " " << user.getState() << endl;
	}
	cout << "===========groups============= " << endl;
	for (Group& group : g_currentUserGroupList)
	{
		cout << "group id: " << group.getId() << " group name: " << group.getName() << " group desc: " << group.getDesc() << endl;
		cout << "group users: " << endl;
		for (GroupUser& user : group.getUsers())
		{
			cout << user.getId() << " " << user.getName() << " " << user.getRole() << endl;
		}
	}
}

void readTaskHandler(int clientfd)
{
	while (1)
	{
		string buffer = receiveFrame(clientfd);
        int len = buffer.empty() ? -1 : static_cast<int>(buffer.size());
		if (-1 == len || 0 == len)
		{
			close(clientfd);
			exit(-1);
		}
		json js = json::parse(buffer);
		if (ONE_CHAT_MSG == js["msgid"].get<int>())
		{
			cout << js["time"].get<string> ()<<" " << "[" << js["id"] << "]:" << js["name"] << "said" << js["msg"] << endl;
				continue;
		}
		else if (GROUP_CHAT_MSG == js["msgid"].get<int>())
		{
			cout << js["time"].get<string>() << " " << "[" << js["id"] << "]:" << js["name"] << "said to group " << js["groupid"] << ":" << js["msg"] << endl;
			continue;
		}
	}
}

string getCurrentTime()
{
	time_t now = time(nullptr);
    tm local{};
    localtime_r(&now, &local);
    char text[32];
    strftime(text, sizeof(text), "%Y-%m-%d %H:%M:%S", &local);
    return text;
}
unordered_map<string, function<void(int, string)>>commandHandlerMap = {
	{"help",help},
	{"chat",chat },
	{"addfriend",addFriend },
	{"creategroup",createGroup },
	{"addgroup",addGroup },
	{"groupchat",groupchat},
	{"loginout",loginout}

};
unordered_map<string,string>commandMap = {
	{"help","显示帮助信息，命令格式help"},
	{"chat","一对一聊天，命令格式chat:friendid:message"},
	{"addfriend","添加好友，命令格式addfriend:friendid"},
	{"creategroup","创建群组，命令格式creategroup:groupname:groupdesc"},
	{"addgroup","加入群组，命令格式addgroup:groupid"},
	{"groupchat","群聊，命令格式groupchat:groupid:message"},
	{"loginout","退出系统，命令格式loginout"}
};
void mainMenu(int clientfd)
{
	help();
	char buffer[1024] = { 0 };
	while(isMenuRunning)
	{
		cin.getline(buffer, 1024);
		string commandbuf(buffer);
		string command;
		int idx = commandbuf.find(":");
		if (idx != string::npos)
		{
			command = commandbuf.substr(0, idx);
		}
		else
		{
			command = commandbuf;
		}
		auto it = commandHandlerMap.find(command);
		if (it == commandHandlerMap.end())
		{
			cerr << "invalid command!" << endl;
			continue;
		}

		//调用相应的命令处理函数
		it->second(clientfd, commandbuf.substr(idx + 1));
	}
}

void help(int fd, string str)
{
	cout << "show command list:" << endl;
	for (auto& p : commandMap)
	{
		cout << p.first << ":" << p.second << endl;
	}
	cout << endl;
}

void chat(int fd, string str)
{
	int idx = str.find(":");//friendid:message
	if (idx == string::npos)
	{
		cerr << "invalid command!" << endl;
		return;
	}
	int friendid = atoi(str.substr(0, idx).c_str());
	string message = str.substr(idx + 1);
	string time = getCurrentTime();
	json js;
	js["msgid"] = ONE_CHAT_MSG;
	js["id"] = g_currentUser.getId();
	js["name"] = g_currentUser.getName();
	js["toid"] = friendid;
	js["msg"] = message;
	js["time"] = time;

	string request = js.dump();
	int len = send(fd, request.c_str(), strlen(request.c_str()) + 1, 0);
	if (-1 == len)
	{
		cerr << "send chat msg error!" << endl;
	}
}

void addFriend(int fd, string str)
{
	int friendid = atoi(str.c_str());

	//判断是否已经是好友关系


	//封装添加好友的json数据
	json js;
	js["msgid"] = ADD_FRIEND_MSG;
	js["id"] = g_currentUser.getId();
	js["friendid"] = friendid;
	//发送添加好友的请求
	send(fd, js.dump().c_str(), strlen(js.dump().c_str()) + 1, 0);


	//接收添加好友的响应

}

void createGroup(int fd, string str)
{
	//解析命令参数，获取groupname和groupdesc
	string groupname;
	string groupdesc;
	int idx = str.find(":");
	if (idx == -1)
	{
		cerr << "invalid command!" << endl;
	}
	groupname = str.substr(0, idx);
	groupdesc = str.substr(idx + 1);

	//封装创建群组的json数据
	json js;
	js["msgid"] = CREATE_GROUP_MSG;
	js["id"] = g_currentUser.getId();
	js["groupname"] = groupname;
	js["groupdesc"] = groupdesc;
	//发送创建群组的请求
	int len = send(fd, js.dump().c_str(), strlen(js.dump().c_str()) + 1, 0);
	if(-1==len)
	{
		cerr << "send create group msg error!" << endl;
	}

}

void addGroup(int fd, string str)
{
	int groupid = atoi(str.c_str());
	//封装加入群组的json数据
	json js;
	js["msgid"] = ADD_GROUP_MSG;
	js["id"] =g_currentUser.getId() ;
	js["groupid"] = groupid;
	string buffer = js.dump();
	int len = send(fd,buffer.c_str(),strlen(buffer.c_str())+1,0);
	if (-1 == len)
	{
		cerr << "send add group msg error!" << endl;
	}
}

void groupchat(int fd, string str)
{
	int idx = str.find(":");
	if (idx == -1)
	{
		cerr << "invalid command!" << endl;
		return;
	}
	int groupid = atoi(str.substr(0, idx).c_str());
	string message = str.substr(idx + 1);
	string time = getCurrentTime();
	json js;
	js["msgid"] = GROUP_CHAT_MSG;
	js["id"] = g_currentUser.getId();
	js["name"] = g_currentUser.getName();
	js["groupid"] = groupid;
	js["msg"] = message;
	js["time"] = time;
	int len = send(fd, js.dump().c_str(), strlen(js.dump().c_str()) + 1, 0);
	if (-1 == len)
	{
		cerr << "send group chat msg error!" << endl;
		return;
	}

}

void loginout(int fd, string str)
{
	//封装退出系统的json数据
	json js;
	js["msgid"] = LOGIN_OUT_MSG;
	js["id"] = g_currentUser.getId();
	string buffer = js.dump();
	int len = send(fd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
	isMenuRunning = false;
	if (-1 == len)
	{
		cerr << "send quit msg error!" << endl;
	}
	else
	{
		cout << "bye" << endl;
		exit(0);
	}
}
