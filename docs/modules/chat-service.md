# 业务层与用户会话

此分支以现有完整工程为起点，整理本模块的实际实现和后续开发范围。

## 源码入口

| 文件 | 职责 |
| --- | --- |
| [chatserver/src/server/chatservice.cpp](../../chatserver/src/server/chatservice.cpp) | 处理器映射、身份校验和消息投递 |
| [chatserver/include/chatservice.h](../../chatserver/include/chatservice.h) | ChatService 与共享连接表 |
| [chatserver/include/public.h](../../chatserver/include/public.h) | 注册登录和聊天消息类型 |

## 当前调用与处理关系

`getHandler()` 按消息编号选择业务处理器，并根据真实 TCP 连接识别已登录用户，覆盖请求中的发送者 ID。注册和登录允许在未认证连接上执行；其他业务要求登录。

登录通过密码校验和 `claimOnline()` 后记录连接，等待 Redis 订阅确认再回复。单聊调用 `deliver()`，群聊先查成员、排除发送者，再逐用户调用相同投递函数。本机连接直接发送，其他用户尝试 Redis 发布，失败或无订阅者时保存离线消息。

连接断开或主动注销，只删除本进程拥有的用户映射和订阅。优雅退出不能代替崩溃恢复。

## 后续工作

- [ ] 补群成员权限、请求字段验证和可信发送者名称。
- [ ] 为建群、入群和好友操作增加明确回执。
- [ ] 设计会话租约及登录期间的待发消息队列。

## 验证与边界

现有测试验证重复登录、跨服单聊群聊、按连接身份注销和本进程退出隔离；当前没有好友审批、完整权限控制或会话租约。

[返回开发流程](../../CONTRIBUTING.md) · [返回项目说明](../../README.md)
