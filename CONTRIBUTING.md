# 模块分支与开发流程

`main` 保存可编译的集成版本。模块分支从现有集成版本建立，供后续按模块学习、修改和提交 PR 使用；初始提交整理模块设计、调用关系与待改进项，提交日期为实际操作时间。

## 模块分支

| 分支 | 范围 | 分支中的说明 |
| --- | --- | --- |
| [module/network](https://github.com/zhouNorth/ChatServer/tree/module/network) | Muduo 网络层、协议分帧 | `docs/modules/network.md` |
| [module/chat-service](https://github.com/zhouNorth/ChatServer/tree/module/chat-service) | 身份校验、登录、单聊、好友及群聊 | `docs/modules/chat-service.md` |
| [module/mysql-storage](https://github.com/zhouNorth/ChatServer/tree/module/mysql-storage) | MySQL 包装、数据模型与离线存储 | `docs/modules/mysql-storage.md` |
| [module/redis-routing](https://github.com/zhouNorth/ChatServer/tree/module/redis-routing) | 用户频道、跨服路由与订阅重连 | `docs/modules/redis-routing.md` |
| [module/nginx-deploy](https://github.com/zhouNorth/ChatServer/tree/module/nginx-deploy) | Nginx stream、双节点配置与启动 | `docs/modules/nginx-deploy.md` |
| [module/client](https://github.com/zhouNorth/ChatServer/tree/module/client) | 客户端命令、收发线程与消息显示 | `docs/modules/client.md` |
| [module/integration-tests](https://github.com/zhouNorth/ChatServer/tree/module/integration-tests) | 隔离测试、故障场景与验证边界 | `docs/modules/integration-tests.md` |

各模块仍包含完整工程，可以查看相邻层的调用关系。分支之间通过 Git 隔离修改，不是把源码拆成互相无法构建的目录快照。

## 开始一次修改

```bash
git clone https://github.com/zhouNorth/ChatServer.git
cd ChatServer
git fetch origin
git switch --track origin/module/network
# 从对应模块分支创建本次修改分支，例如：
git switch -c fix/network-request-validation
```

每次提交对应一个实际变更：说明问题、修改范围和验证结果。修复使用 `fix:`，新增功能使用 `feat:`，文档使用 `docs:`，测试使用 `test:`。构建产物、依赖、日志和本地配置不提交。

## 验证与合并

仅修改文档时，检查路径、示例和实现描述是否准确。修改源码时先构建，再按变更范围运行测试：

```bash
bash scripts/build-linux.sh
# 已安装测试所需 MySQL 服务端二进制时：
sudo python3 -u tests/cluster_integration.py
```

完成后推送工作分支并创建 PR，描述具体行为、测试结果和剩余限制。合并到 `main` 后，将新的集成版本同步到后续开发分支。模块分支中的待办仅表示计划，不能作为已实现功能引用。
