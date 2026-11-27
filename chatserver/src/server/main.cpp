#include "../../include/chatserver.h"
#include "../../include/chatservice.h"
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
namespace {
volatile std::sig_atomic_t stopping = 0;
void requestStop(int) { stopping = 1; }
}
int main(int argc, char** argv) {
    try {
        if (argc > 3) throw std::runtime_error("Usage: ChatServer [bind-ip] [port]");
        std::string host = argc > 1 ? argv[1] : "127.0.0.1";
        int port = argc > 2 ? std::stoi(argv[2]) : 6000;
        if (port < 1 || port > 65535) throw std::runtime_error("Invalid listen port");
        std::signal(SIGINT, requestStop);
        std::signal(SIGTERM, requestStop);
        EventLoop loop;
        ChatService* service = ChatService::instance();
        if (!service->start(&loop)) {
            std::cerr << "Redis unavailable; check CHAT_REDIS_HOST/PORT/PASSWORD\n";
            return 1;
        }
        InetAddress address(host, static_cast<uint16_t>(port));
        ChatServer server(&loop, address, "ChatServer-" + std::to_string(port));
        loop.runEvery(0.2, [&loop] { if (stopping) loop.quit(); });
        server.start();
        std::cout << "ChatServer listening " << host << ":" << port << std::endl;
        loop.loop();
        service->reset();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << "\n";
        return 1;
    }
}
