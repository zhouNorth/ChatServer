# 隔离集成测试与验证范围

此分支以现有完整工程为起点，整理本模块的实际实现和后续开发范围。

## 源码入口

| 文件 | 职责 |
| --- | --- |
| [tests/cluster_integration.py](../../tests/cluster_integration.py) | 真实 MySQL Redis Nginx 和双服务端测试 |
| [deploy/schema.sql](../../deploy/schema.sql) | 临时数据库初始化 |
| [deploy/nginx.conf](../../deploy/nginx.conf) | 测试复制并替换端口的代理配置 |
| [README-Cluster.md](../../README-Cluster.md) | 测试依赖与运行方式 |

## 当前调用与处理关系

测试创建临时 MySQL 数据目录、独立 Redis、随机端口、两个 ChatServer 和 Nginx，并在结束时清理自己启动的进程。已有业务数据库不参与测试。

用真实 socket 验证双向单聊、群聊、重复登录、离线消息、分帧、注销退订和 Redis 恢复，并运行编译后的 C++ 客户端。最后核对 Nginx 日志中的用户连接实际分配到不同后端。

执行命令为 `sudo python3 -u tests/cluster_integration.py`，需要先构建且已安装 MySQL 服务端二进制。

## 后续工作

- [ ] 增加未认证请求、群权限及非法字段断言。
- [ ] 增加 SIGKILL、半开连接、Redis 延迟和投递窗口故障测试。
- [ ] 建立独立压测脚本，记录硬件、并发、消息大小和延迟分位数。

## 验证与边界

功能测试通过不能证明没有全部竞态，也不能替代性能压测。本分支目前整理测试设计，没有新增或虚构测试运行结果。

[返回开发流程](../../CONTRIBUTING.md) · [返回项目说明](../../README.md)
