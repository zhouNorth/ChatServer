# 客户端命令与收发线程

此分支以现有完整工程为起点，整理本模块的实际实现和后续开发范围。

## 源码入口

| 文件 | 职责 |
| --- | --- |
| [chatserver/src/client/main.cpp](../../chatserver/src/client/main.cpp) | 命令解析、JSON 封装、收发和显示 |
| [chatserver/include/public.h](../../chatserver/include/public.h) | 与服务端共享的消息编号 |
| [chatserver/include/server/group.hpp](../../chatserver/include/server/group.hpp) | 群组与成员列表数据对象 |

## 当前调用与处理关系

客户端启动后建立 TCP 连接，主线程同步执行注册或登录请求及回复接收。登录成功后，主线程运行 `mainMenu()` 处理输入，子线程 `readTaskHandler()` 接收聊天消息。

`receiveFrame()` 累积字节、按换行提取完整 JSON；接收权由登录阶段的主线程移交接收线程。发送请求兼容原 NUL 分隔格式。

断线后当前关闭 socket 并退出进程，没有自动重连。`loginout` 发送注销消息后也退出，不是回到登录菜单。

## 后续工作

- [ ] 循环处理部分发送、EINTR 和发送错误。
- [ ] 设计可停止并 join 的接收线程及统一连接状态机。
- [ ] 增加退避重连、重新认证和缓存状态刷新。
- [ ] 完善错误回复显示与输入长度校验。

## 验证与边界

现有测试运行真实 C++ 客户端完成注册、较大登录回复和退出；尚未实现本地聊天历史保存或自动恢复会话。

[返回开发流程](../../CONTRIBUTING.md) · [返回项目说明](../../README.md)
