# Redis 用户频道与跨服务端路由

此分支以现有完整工程为起点，整理本模块的实际实现和后续开发范围。

## 源码入口

| 文件 | 职责 |
| --- | --- |
| [chatserver/include/server/redis.hpp](../../chatserver/include/server/redis.hpp) | 发布订阅接口与线程同步状态 |
| [chatserver/src/server/redis.cpp](../../chatserver/src/server/redis.cpp) | hiredis 连接、观察线程和重连 |
| [chatserver/src/server/chatservice.cpp](../../chatserver/src/server/chatservice.cpp) | 订阅生命周期、deliver 和 receiveRedis |

## 当前调用与处理关系

默认频道由 `chat:user:` 加用户 ID 构成。每个 ChatServer 只订阅本机登录用户频道，发送方按接收者 ID 生成同名频道，不需要知道目标服务端地址。

发布连接由互斥锁保护，订阅连接由观察线程独占。观察线程比较期望集合、发送订阅或退订命令、处理 Redis 回复，并通过 `queueInLoop()` 将业务处理交还主 EventLoop。

订阅断线后重连并恢复仍需要的用户频道；发布连接失败后释放，在下次发布时尝试重新建立。不盲目重试已经执行但可能丢失回复的同一次发布。

## 后续工作

- [ ] 区分订阅连接状态、订阅确认和终端消息确认。
- [ ] 评估持久化消息 ID、ACK、重试和去重。
- [ ] 按部署需求增加 ACL 用户名和 TLS 支持。

## 验证与边界

已覆盖 Redis 重启后的订阅恢复和后续跨服转发。Pub/Sub 不提供断线重放，PUBLISH 返回订阅者数量不是最终送达确认。

[返回开发流程](../../CONTRIBUTING.md) · [返回项目说明](../../README.md)
