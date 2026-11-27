# 在 Linux / WSL 中构建

Redis 跨服务端转发和 Nginx 双节点部署，请优先阅读 [集群运行说明](README-Cluster.md)。

这个项目生成 Linux 程序 `ChatServer` 和 `ChatClient`。
服务端依赖 muduo、Boost、hiredis 和 MySQL 客户端库，并需要可用的 Redis 服务。
JSON 头文件已包含在项目内。

## 首次安装并编译

在 Ubuntu 的终端中进入本项目根目录，运行：

```bash
bash scripts/setup-linux.sh
```

脚本通过 apt 安装编译器、CMake、Boost、MySQL 开发库和 GDB，
从 https://github.com/chenshuo/muduo 获取 v2.0.2，
将 muduo 安装在项目的 `.deps/muduo-install` 下，然后编译本项目。

以后只需：

```bash
bash scripts/build-linux.sh
```

生成文件位于 `chatserver/bin/ChatServer` 和 `chatserver/bin/ChatClient`。

## 使用 VS Code

安装本项目推荐的 WSL、C/C++、CMake Tools 扩展。
通过命令面板的 `WSL: Reopen Folder in WSL` 打开本项目。
执行首次安装脚本后，运行 `CMake: Configure`，构建配置会自动寻找
本项目 `.deps/muduo-install` 下的 muduo 库。
按 Ctrl+Shift+B 编译。调试配置必须在 WSL 窗口中使用。

## 运行

程序需要本地 MySQL 服务和已有的 `chat` 数据库。
连接参数位于 `chatserver/include/server/db/db.h`，默认地址为
`127.0.0.1:3306`，用户为 `root`。请将密码改为实际本地数据库密码。
编译成功只说明程序已经生成，并不表示数据库已经配置完成。

在两个 Linux 终端中分别运行：

```bash
./chatserver/bin/ChatServer
```

```bash
./chatserver/bin/ChatClient 127.0.0.1 6000
```

源码统一使用 UTF-8，数据库连接使用 utf8mb4，以便 JSON 正确传输中文。
修改前的源码保存在 `.codex-backups` 下。
