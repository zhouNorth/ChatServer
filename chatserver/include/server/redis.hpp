#pragma once
#include <hiredis/hiredis.h>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <set>
#include <string>
#include <thread>

// 发布连接由锁保护；订阅连接仅由观察线程操作，避免并发访问 hiredis context。
class Redis {
public:
    using Handler = std::function<void(int, const std::string&)>;
    Redis() = default;
    ~Redis();
    Redis(const Redis&) = delete;
    Redis& operator=(const Redis&) = delete;
    bool start(Handler handler);
    void stop();
    bool subscribe(int userid);
    void unsubscribe(int userid);
    // -1: Redis 不可用；0: 没有订阅者；正数: 收到发布的订阅连接数量。
    long long publish(int userid, const std::string& message);
private:
    redisContext* connect();
    void observe();
    std::string channel(int userid) const;
    std::atomic<bool> running_{false};
    std::thread observer_;
    Handler handler_;
    std::mutex commandMutex_, subscriptionsMutex_;
    std::condition_variable changed_;
    std::set<int> desired_, acknowledged_;
    bool connected_ = false;
    redisContext* publisher_ = nullptr;
    std::string host_, password_, prefix_;
    int port_ = 6379;
};
