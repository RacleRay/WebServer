#pragma once

#include <functional>
#include <memory>
#include <string>
#include <sys/epoll.h>
#include <unordered_map>

#include "Timer.h"


class EventLoop;
class HttpData;


class Channel {
private:
    using Callback = std::function<void()>;

    EventLoop* m_event_loop;
    int m_fd{0};

    // 初始值会由使用Channel的调用者设置，Server::handle_available_connfd 中 accept 后，
    // 新 Http 连接的 m_events 调用 HttpData::add_new_event 设置初始值
    // Http 负责设置 Http 的事件，这很 nice
    uint32_t m_events{0};
    uint32_t m_revents{0};  // returned from epoll_wait()
    uint32_t m_last_events{0}; // 上一个 epoll_add 的监听事件

    std::weak_ptr<HttpData> m_owner_http;

    Callback pr_read_handler;
    Callback pr_write_handler;
    Callback pr_conn_handler;
    Callback pr_error_handler;

public:
    explicit Channel(EventLoop* loop): m_event_loop(loop) {};
    Channel(EventLoop* loop, int fd): m_event_loop(loop), m_fd(fd) {};
    ~Channel() = default;

    [[nodiscard]] int get_fd() const noexcept {
        return m_fd;
    }
    void set_fd(int fd) noexcept {
        m_fd = fd;
    }

    void set_owner_http(const std::shared_ptr<HttpData>& owner) noexcept {
        m_owner_http = owner;
    }
    
    std::shared_ptr<HttpData> get_owner_http() {
        std::shared_ptr<HttpData> ret{m_owner_http.lock()};
        return ret;
    }

    void set_read_handler(Callback&& cb) noexcept {
        pr_read_handler = std::move(cb);
    }

    void set_write_handler(Callback&& cb) noexcept {
        pr_write_handler = std::move(cb);
    }

    void set_conn_handler(Callback&& cb) noexcept {
        pr_conn_handler = std::move(cb);
    }

    void set_error_handler(Callback&& cb) noexcept {
        pr_error_handler = std::move(cb);
    }

    // 准备 add 到 epoll 的 events
    void set_events(uint32_t ev) noexcept {
        m_events = ev;
    }

    // epoll wait 返回后得到的 revents 
    void set_revents(uint32_t rev) noexcept {
        m_revents = rev;
    }

    uint32_t &get_events() noexcept {
        return m_events;
    }

    [[nodiscard]] uint32_t get_lastevents() const noexcept {
        return m_last_events;
    }

    bool compare_and_updata_last_evt() noexcept {
        bool is_same = (m_last_events == m_events);
        m_last_events = m_events;
        return is_same;
    }

    void handle_read() {
        if (pr_read_handler) {
            pr_read_handler();
        }
    }

    void handle_write() {
        if (pr_write_handler) {
            pr_write_handler();
        }
    }

    void handle_connect() {
        if (pr_conn_handler) {
            pr_conn_handler();
        }
    }

    void handle_error() {
        if (pr_error_handler) {
            pr_error_handler();
        }
    }

    void handle_revents();
};