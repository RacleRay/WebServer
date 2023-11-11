#pragma once

#include <cassert>
#include <memory>
#include <sys/epoll.h>
#include <unordered_map>
#include <vector>

#include "Channel.h"
#include "HttpData.h"
#include "Timer.h"



class Epoll {
public:
    Epoll();
    ~Epoll() = default;

    // epoll event management
    void epoll_add(std::shared_ptr<Channel> req_channel, int timeout);
    void epoll_mod(std::shared_ptr<Channel> req_channel, int timeout);
    void epoll_del(std::shared_ptr<Channel> req_channel);

    void add_timer(std::shared_ptr<Channel> req_channel, int timeout);
    void handle_expired();

    [[nodiscard]] int get_epoll_fd() const noexcept { return m_epoll_fd; }

    std::vector<std::shared_ptr<Channel>> get_active_events();
    std::vector<std::shared_ptr<Channel>> collect_active_channels(int event_count);

private:
    static const int MAX_FDS = 100'000;
    
    int m_epoll_fd;

    // events management
    std::vector<epoll_event> m_events_buf;

    // fd to Channel management
    std::shared_ptr<Channel> m_fd2channels[MAX_FDS];

    // fd to HttpData management
    std::shared_ptr<HttpData> m_fd2httpdata[MAX_FDS];

    // timer management
    TimerManager m_timer_manager;
};