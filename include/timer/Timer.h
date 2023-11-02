#pragma once

#include <deque>
#include <memory>
#include <queue>
#include <sys/time.h>
#include <unistd.h>

#include "HttpData.h"
#include "Mutex.h"
#include "noncopyable.h"


class HttpData;

// For priority queue.
class TimerNode : public std::enable_shared_from_this<TimerNode> {
public:
    TimerNode(std::shared_ptr<HttpData> request_data_sp, int timeout);
    ~TimerNode();

    TimerNode(const TimerNode &tn);

    std::shared_ptr<TimerNode> get_shared() {
        return shared_from_this();
    }

    bool is_valid();

    void update(int timeout);
    void clear_request();
    
    void set_deleted() noexcept { m_deleted = true; }
    [[nodiscard]] bool is_deleted() const noexcept { return m_deleted; }
    [[nodiscard]] size_t get_expire_time() const noexcept { return m_expire_time; }

private:
    bool m_deleted {false};  // target
    size_t m_expire_time;   // time that timer is expired
    std::shared_ptr<HttpData> m_http_data_sp;
};


struct TimerCmp {
    bool operator()(std::shared_ptr<TimerNode> &a, std::shared_ptr<TimerNode> &b) const noexcept {
        return a->get_expire_time() > b->get_expire_time();
    }
};


class TimerManager {
public:
    TimerManager() = default;
    ~TimerManager() = default;

    void add_timer(const std::shared_ptr<HttpData>& request_data_sp, int timeout);
    void handle_expired_event();

private:
    using TimerNodeSP = std::shared_ptr<TimerNode>;

    std::priority_queue<TimerNodeSP, std::deque<TimerNodeSP>, TimerCmp> m_timer_queue;
};

