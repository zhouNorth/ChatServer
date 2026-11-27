# Nginx + Redis 双服务端聊天

当前版本支持两个或多个 ChatServer 共享同一 MySQL 和 Redis。
默认演示拓扑：

```text
ChatClient -> Nginx 127.0.0.1:8000
                 |-> ChatServer 127.0.0.1:6001
                 |-> ChatServer 127.0.0.1:6002
                            |-> 共享 MySQL
                            |-> 共享 Redis
```

## 1. 安装和编译

在 Ubuntu / WSL 的项目根目录运行：

```bash
bash scripts/setup-linux.sh
```

脚本安装 hiredis、Redis、Nginx stream 模块等依赖，编译
`chatserver/bin/ChatServer` 和 `chatserver/bin/ChatClient`。
以后修改源码只需要 `bash scripts/build-linux.sh`。

## 2. 配置共享数据库和 Redis

沿用自己的 MySQL 服务，设置正确账号、密码和数据库。
不要直接操作其他项目的数据库。全新数据库可使用
`deploy/schema.sql` 创建本项目需要的五张表。
该 SQL 不会迁移已有表的结构，已有表须与源码字段顺序一致。

```bash
export CHAT_DB_HOST=127.0.0.1
export CHAT_DB_PORT=3306
export CHAT_DB_USER=root
export CHAT_DB_PASSWORD='填写你的数据库密码'
export CHAT_DB_NAME=chat

# 全新 chat 数据库已创建时，导入表结构；mysql 会交互式询问密码。
mysql -h "$CHAT_DB_HOST" -P "$CHAT_DB_PORT" -u "$CHAT_DB_USER" -p "$CHAT_DB_NAME" < deploy/schema.sql
```

如果本机没有 Redis 正在运行，可启动一个仅监听本机的开发实例：

```bash
redis-server --bind 127.0.0.1 --port 6379 --daemonize yes
redis-cli -p 6379 ping
```

两个服务端必须使用相同的 Redis 地址、端口和频道前缀：

```bash
export CHAT_REDIS_HOST=127.0.0.1
export CHAT_REDIS_PORT=6379
export CHAT_REDIS_PREFIX=chat:user:
# Redis 设置密码时再填写：
# export CHAT_REDIS_PASSWORD='你的 Redis 密码'
```

上述环境变量必须在启动集群的同一个终端设置。开发默认值保留原项目的
MySQL `root/123456/chat`，Redis 默认无需认证；请按实际环境覆盖。
Redis 不可连接时，ChatServer 启动会报错退出。

## 3. 启动集群

```bash
bash scripts/run-cluster.sh
```

启动器先验证 Nginx 配置，再启动两个 ChatServer 和本项目专用的 Nginx
实例，不替换系统 `/etc/nginx/nginx.conf`。
保持终端运行；Ctrl+C 会停止该启动器管理的服务端和 Nginx。

在另外两个终端分别运行：

```bash
./chatserver/bin/ChatClient 127.0.0.1 8000
```

分别注册、登录后使用 `chat:用户ID:内容` 发送消息。
Nginx 使用 least_conn 将新连接分配给后端；连接建立后不会逐消息切换。
以下日志可以核对真实路由：

```bash
tail -f .run/access.log .run/server-6001.log .run/server-6002.log
```

Nginx 的 stream access log 通常在会话结束时写入。
默认前端和后端都仅监听本机；跨机器部署时需调整 `deploy/nginx.conf`
的监听地址和 upstream，并设置共享数据库、Redis 的地址和访问权限。
Nginx 支持连接建立失败时尝试另一个后端，不能迁移已经断开的 TCP 会话。
空闲连接超时为一小时，当前没有应用层心跳。

也可以手动在继承同一组环境变量的两个终端启动：

```bash
./chatserver/bin/ChatServer 127.0.0.1 6001
./chatserver/bin/ChatServer 127.0.0.1 6002
```

## 4. 消息和线程流程

* 登录：校验密码 → MySQL 条件更新 `offline -> online`，防止并发重复登录
  → 保存本机连接 → 等待 Redis `SUBSCRIBE chat:user:<id>` 确认 → 回复登录成功。
* 单聊、群聊：本机存在目标连接则直接发送；否则 `PUBLISH` 到目标用户频道。
  没有订阅者或 Redis 发布失败时，将消息保存到 MySQL `offlinemsg`。
  Redis 的订阅关系承担路由，不依赖数据库 online 状态判断远端是否可达。
* 接收：每个服务端一个 Redis 观察线程，独占订阅连接；收到消息后将回调排入
  muduo 主 EventLoop，再查找本机用户连接并发送。目标已退出则保存离线消息。
* 发布使用独立 hiredis 连接并加锁；订阅线程根据期望用户集合更新订阅。
  断线后重连，并恢复仍在本机登录的用户订阅。
* 注销、连接断开：按真实连接识别用户，删除本机连接、退订、更新该用户为离线。
* SIGINT / SIGTERM：信号处理器只设置退出标记，事件循环结束后执行清理。
  仅清理本进程拥有的用户，不再全表更新其他服务器的在线状态。

服务端当前仍为单个网络 EventLoop，加一个 Redis 观察线程。
MySQL 访问仍是同步操作，没有数据库连接池或独立业务线程池。
网络协议使用一行一个 JSON，服务端同时兼容原客户端的 NUL 分隔请求。
客户端和服务端均累积半包、拆分多包；单条协议消息上限为 1 MiB。

## 5. 运行隔离的集成测试

测试还需要 MySQL 服务端二进制（普通运行仅需要客户端开发库）：

```bash
sudo apt-get install -y default-mysql-server
sudo python3 -u tests/cluster_integration.py
```

测试自动创建临时 MySQL 数据目录、独立 Redis、随机端口、两个服务端和
Nginx。不会使用已有业务数据库；结束后关闭自己启动的进程并清理临时目录。
覆盖跨服务端双向单聊、群聊、重复登录、离线消息、原生 C++ 客户端、
大消息、拆包粘包、退出退订、Redis 重连恢复和单进程退出隔离。
通过 Nginx 日志核对两个聊天用户实际使用不同后端。

## 6. 当前可靠性边界

Redis Pub/Sub 是尽力实时转发，不保证持久化或客户端确认。
发布返回的订阅者数量不是“接收者已收到”的确认。Redis 接收端掉线、
进程在收到订阅消息后崩溃时仍可能丢消息；发布响应丢失时保存离线消息
也可能造成重复。本版本没有声称实现 exactly-once、确认重试或历史消息存储。
官方说明：https://redis.io/docs/latest/develop/pubsub/

离线消息仍沿用原项目的“登录查询后删除”，没有客户端 ACK，
查询与删除间的并发写入也没有事务保护。在线消息没有独立历史表。

SIGKILL、机器断电等无法执行清理的故障会留下 MySQL online 状态。
当前没有分布式会话租约；重连可能被判断为重复登录。确认该用户旧会话
已经不存在后，可由管理员仅修正该用户的状态，不能批量重置其他在线用户：

```sql
UPDATE user SET state='offline' WHERE id=需要恢复的用户ID;
```

原有密码明文存储、业务 SQL 拼接和权限校验尚未全面改造。
本次给离线消息增加了 SQL 转义，并根据连接覆盖请求里的发送者 ID；
生产使用前仍需完善身份、群成员权限、慢客户端控制和可靠投递设计。

Nginx stream 代理说明：https://nginx.org/en/docs/stream/ngx_stream_proxy_module.html
