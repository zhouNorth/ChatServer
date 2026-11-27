# Nginx TCP 代理与集群部署

此分支以现有完整工程为起点，整理本模块的实际实现和后续开发范围。

## 源码入口

| 文件 | 职责 |
| --- | --- |
| [deploy/nginx.conf](../../deploy/nginx.conf) | stream 代理、least_conn 和两个后端 |
| [scripts/setup-linux.sh](../../scripts/setup-linux.sh) | Linux 依赖安装与 muduo 构建 |
| [scripts/build-linux.sh](../../scripts/build-linux.sh) | CMake 配置与编译 |
| [scripts/run-cluster.sh](../../scripts/run-cluster.sh) | 双 ChatServer 和项目专用 Nginx 启停 |
| [README-Cluster.md](../../README-Cluster.md) | 环境配置、启动步骤和可靠性边界 |

## 当前调用与处理关系

开发拓扑为客户端连接 `127.0.0.1:8000`，Nginx 按 least_conn 将新 TCP 连接分给 `6001` 或 `6002`。连接建立后不按聊天消息重新选择后端。

两个服务端必须配置共享 MySQL、Redis 和一致的频道前缀。启动器使用项目专用 Nginx 配置与 `.run` 日志目录，不替换系统 Nginx 配置。

连接后端失败可尝试其他节点，但已经建立的会话不能迁移。应用目前没有心跳，也不能把代理 keepalive 作为自动重连功能。

## 后续工作

- [ ] 增加显式服务就绪检查和可配置端口。
- [ ] 补充容器或 systemd 部署及日志管理。
- [ ] 测试后端退出、空闲超时和客户端重连策略。

## 验证与边界

现有测试用 Nginx access log 确认两个测试用户连接到不同后端，验证真实跨服消息路径；这不是生产容量或高可用指标。

[返回开发流程](../../CONTRIBUTING.md) · [返回项目说明](../../README.md)
