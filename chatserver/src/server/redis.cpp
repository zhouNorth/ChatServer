#include "server/redis.hpp"
#include <muduo/base/Logging.h>
#include <chrono>
#include <cstdlib>
#include <memory>
#include <poll.h>
#include <stdexcept>

namespace {
std::string env(const char* name, const char* fallback) {
    const char* value = std::getenv(name);
    return value ? value : fallback;
}
using Reply = std::unique_ptr<redisReply, decltype(&freeReplyObject)>;
}

Redis::~Redis() { stop(); }
std::string Redis::channel(int userid) const { return prefix_ + std::to_string(userid); }

redisContext* Redis::connect() {
    timeval timeout{2, 0};
    redisContext* context = redisConnectWithTimeout(host_.c_str(), port_, timeout);
    if (!context) return nullptr;
    if (context->err || redisSetTimeout(context, timeout) != REDIS_OK) {
        redisFree(context);
        return nullptr;
    }
    if (!password_.empty()) {
        Reply reply(static_cast<redisReply*>(redisCommand(context, "AUTH %b",
                    password_.data(), password_.size())), freeReplyObject);
        if (!reply || reply->type == REDIS_REPLY_ERROR) {
            redisFree(context);
            return nullptr;
        }
    }
    return context;
}

bool Redis::start(Handler handler) {
    host_ = env("CHAT_REDIS_HOST", "127.0.0.1");
    port_ = std::stoi(env("CHAT_REDIS_PORT", "6379"));
    if (port_ < 1 || port_ > 65535) throw std::runtime_error("Invalid Redis port");
    password_ = env("CHAT_REDIS_PASSWORD", "");
    prefix_ = env("CHAT_REDIS_PREFIX", "chat:user:");
    handler_ = std::move(handler);
    publisher_ = connect();
    if (!publisher_) return false;
    running_ = true;
    observer_ = std::thread(&Redis::observe, this);
    std::unique_lock<std::mutex> lock(subscriptionsMutex_);
    bool ready = changed_.wait_for(lock, std::chrono::seconds(4), [this] { return connected_; });
    lock.unlock();
    if (!ready) stop();
    return ready;
}

void Redis::stop() {
    running_ = false;
    changed_.notify_all();
    if (observer_.joinable()) observer_.join();
    std::lock_guard<std::mutex> lock(commandMutex_);
    if (publisher_) redisFree(publisher_);
    publisher_ = nullptr;
}

bool Redis::subscribe(int userid) {
    std::unique_lock<std::mutex> lock(subscriptionsMutex_);
    desired_.insert(userid);
    bool ready = changed_.wait_for(lock, std::chrono::seconds(4), [this, userid] {
        return !running_ || acknowledged_.count(userid);
    });
    if (!ready || !running_) {
        desired_.erase(userid);
        return false;
    }
    return true;
}

void Redis::unsubscribe(int userid) {
    std::unique_lock<std::mutex> lock(subscriptionsMutex_);
    desired_.erase(userid);
    changed_.wait_for(lock, std::chrono::seconds(4), [this, userid] {
        return !running_ || !acknowledged_.count(userid);
    });
}

long long Redis::publish(int userid, const std::string& message) {
    std::lock_guard<std::mutex> lock(commandMutex_);
    if (!publisher_) publisher_ = connect();
    if (!publisher_) return -1;
    std::string name = channel(userid);
    Reply reply(static_cast<redisReply*>(redisCommand(publisher_, "PUBLISH %b %b",
                name.data(), name.size(), message.data(), message.size())), freeReplyObject);
    if (!reply || reply->type != REDIS_REPLY_INTEGER) {
        redisFree(publisher_);
        publisher_ = nullptr;
        // 不盲目重试，避免 Redis 已执行但回复丢失时重复发布。
        return -1;
    }
    return reply->integer;
}

void Redis::observe() {
    while (running_) {
        redisContext* context = connect();
        if (!context) {
            std::unique_lock<std::mutex> lock(subscriptionsMutex_);
            changed_.wait_for(lock, std::chrono::milliseconds(300), [this] { return !running_; });
            continue;
        }
        {
            std::lock_guard<std::mutex> lock(subscriptionsMutex_);
            connected_ = true;
            acknowledged_.clear();
        }
        changed_.notify_all();
        std::set<int> requested;
        bool healthy = true;
        while (running_ && healthy) {
            std::set<int> desired;
            {
                std::lock_guard<std::mutex> lock(subscriptionsMutex_);
                desired = desired_;
            }
            for (int id : desired) if (!requested.count(id)) {
                std::string name = channel(id);
                healthy = redisAppendCommand(context, "SUBSCRIBE %b", name.data(), name.size()) == REDIS_OK;
                if (!healthy) break;
            }
            for (int id : requested) if (healthy && !desired.count(id)) {
                std::string name = channel(id);
                healthy = redisAppendCommand(context, "UNSUBSCRIBE %b", name.data(), name.size()) == REDIS_OK;
            }
            requested = std::move(desired);
            int done = 0;
            while (healthy && !done) healthy = redisBufferWrite(context, &done) == REDIS_OK;
            if (!healthy) break;
            pollfd socket{context->fd, POLLIN, 0};
            int result = ::poll(&socket, 1, 50);
            if (result < 0 || socket.revents & (POLLERR | POLLHUP | POLLNVAL)) break;
            if (result == 0) continue;
            if (redisBufferRead(context) != REDIS_OK) break;
            while (healthy) {
                void* raw = nullptr;
                healthy = redisGetReplyFromReader(context, &raw) == REDIS_OK;
                Reply reply(static_cast<redisReply*>(raw), freeReplyObject);
                if (!reply) break;
                if (reply->type != REDIS_REPLY_ARRAY || reply->elements != 3) continue;
                auto kind = reply->element[0];
                auto name = reply->element[1];
                if (!kind->str || !name->str) continue;
                std::string type(kind->str, kind->len), channelName(name->str, name->len);
                if (channelName.compare(0, prefix_.size(), prefix_) != 0) continue;
                try {
                    int userid = std::stoi(channelName.substr(prefix_.size()));
                    if (type == "subscribe" || type == "unsubscribe") {
                        std::lock_guard<std::mutex> lock(subscriptionsMutex_);
                        if (type == "subscribe") acknowledged_.insert(userid);
                        else acknowledged_.erase(userid);
                        changed_.notify_all();
                    } else if (type == "message" && reply->element[2]->str) {
                        handler_(userid, std::string(reply->element[2]->str, reply->element[2]->len));
                    }
                } catch (const std::exception& error) {
                    LOG_ERROR << "Redis notification: " << error.what();
                }
            }
        }
        redisFree(context);
        {
            std::lock_guard<std::mutex> lock(subscriptionsMutex_);
            connected_ = false;
            acknowledged_.clear();
        }
        changed_.notify_all();
    }
}
