# 数据库访问与离线消息

此分支以现有完整工程为起点，整理本模块的实际实现和后续开发范围。

## 源码入口

| 文件 | 职责 |
| --- | --- |
| [chatserver/src/server/db/db.cpp](../../chatserver/src/server/db/db.cpp) | 连接初始化、SQL 执行及句柄释放 |
| [chatserver/include/server/db/db.h](../../chatserver/include/server/db/db.h) | CHAT_DB_* 环境配置与 MySQL 包装 |
| [chatserver/src/server/usermodel.cpp](../../chatserver/src/server/usermodel.cpp) | 用户查询、注册与条件更新登录状态 |
| [chatserver/src/server/groupmodel.cpp](../../chatserver/src/server/groupmodel.cpp) | 群及成员关系查询 |
| [chatserver/src/server/offlinemessagemodel.cpp](../../chatserver/src/server/offlinemessagemodel.cpp) | 离线 JSON 存取与转义 |
| [deploy/schema.sql](../../deploy/schema.sql) | 五张业务表的初始化结构 |

## 当前调用与处理关系

每次 Model 操作通常构造局部 `MySQL` 对象，连接、同步执行 SQL，最后在析构时关闭连接。当前没有数据库连接池或独立业务线程池。

`claimOnline()` 用 `UPDATE ... WHERE state='offline'` 并检查影响行数，避免两个进程都通过先查再写的登录判断。离线消息保存用户 ID 和 JSON，消息内容写入前进行转义。

登录时查询离线消息后删除，再发送回复；尚无客户端 ACK。在线消息不统一写入历史表。

## 后续工作

- [ ] 为离线消息增加唯一 ID、精确删除和确认状态。
- [ ] 采用参数化 SQL，并完善密码哈希。
- [ ] 将建群和群主登记放入事务。
- [ ] 按实际瓶颈引入连接池和受限业务任务队列。

## 验证与边界

集成测试使用独立临时 MySQL 数据目录，不访问既有业务数据库；覆盖带单引号、UTF-8 和大消息的离线补发。

[返回开发流程](../../CONTRIBUTING.md) · [返回项目说明](../../README.md)
