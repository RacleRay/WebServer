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

    uint32_t m_events{0};  // 初始值会由使用Channel的调用者设置
    uint32_t m_revents{0};  // returned from epoll_wait()
    uint32_t m_last_events{0}; // previous revents

    std::weak_ptr<HttpData> m_owner_http;

    Callback _read_handler;
    Callback _write_handler;
    Callback _conn_handler;
    Callback _error_handler;

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
        _read_handler = std::move(cb);
    }

    void set_write_handler(Callback&& cb) noexcept {
        _write_handler = std::move(cb);
    }

    void set_conn_handler(Callback&& cb) noexcept {
        _conn_handler = std::move(cb);
    }

    void set_error_handler(Callback&& cb) noexcept {
        _error_handler = std::move(cb);
    }

    // 准备 add 到 epoll 的 events
    void set_events(uint32_t ev) noexcept {
        m_events = ev;
    }

    // epoll wait 返回后得到的 revents 
    void set_revents(uint32_t rev) noexcept {
        m_revents = rev;
    }

    [[nodiscard]] uint32_t &get_events() noexcept {
        return m_events;
    }

    [[nodiscard]] uint32_t get_lastevents() const noexcept {
        return m_last_events;
    }

    void updata_last_events() noexcept {
        m_last_events = m_events;
    }

    [[nodiscard]] bool is_events_sameas_last() const noexcept {
        return (m_last_events == m_events);
    }

    void handle_read() const {
        if (_read_handler) {
            _read_handler();
        }
    }

    void handle_write() const {
        if (_write_handler) {
            _write_handler();
        }
    }

    void handle_connect() const {
        if (_conn_handler) {
            _conn_handler();
        }
    }

    void handle_error() const {
        if (_error_handler) {
            _error_handler();
        }
    }

    void handle_events();
};