# 网络层与消息协议

此分支以现有完整工程为起点，整理本模块的实际实现和后续开发范围。

## 源码入口

| 文件 | 职责 |
| --- | --- |
| [chatserver/src/server/main.cpp](../../chatserver/src/server/main.cpp) | 网络启动、Redis 初始化与退出检查 |
| [chatserver/src/server/chatserver.cpp](../../chatserver/src/server/chatserver.cpp) | 连接状态回调与 JSON 消息组帧 |
| [chatserver/include/chatserver.h](../../chatserver/include/chatserver.h) | 网络层对象定义 |
| [chatserver/include/public.h](../../chatserver/include/public.h) | 协议消息编号 |

## 当前调用与处理关系

客户端字节进入 Muduo Buffer 后，`onMessage()` 查找换行或 NUL 分隔符，循环取出完整帧，再解析 JSON 并调用业务处理器。未完成的尾部留在 Buffer 中等待后续数据。单帧限制为 1 MiB。

连接断开时，`onConnection()` 调用业务层清理用户会话。当前只有主 EventLoop，未配置额外 I/O 工作线程；事件回调内的数据库访问会阻塞该线程。

## 后续工作

- [ ] 明确协议错误回复编号，统一到公共枚举。
- [ ] 增加非法字段、超限帧和不完整消息的专项测试。
- [ ] 引入额外 I/O 线程前，设计登录回复与异步聊天消息的发送顺序。

## 验证与边界

现有集成测试覆盖请求拆分、合并、NUL 兼容和大消息；没有完成连接洪泛、慢客户端和高并发性能验证。

[返回开发流程](../../CONTRIBUTING.md) · [返回项目说明](../../README.md)
