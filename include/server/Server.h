#pragma once

#include <memory>

#include "Channel.h"
#include "EventLoop.h"
#include "EventLoopThreadPool.h"


class Server {
public:
    Server(EventLoop* loop, int num_threads, int port);
    ~Server() = default;

    EventLoop* get_loop() { return m_main_loop; }
    void start();
    void handle_available_connfd();
    void handle_connect();

private:
    bool m_started{false};

    EventLoop* m_main_loop;
    int m_num_threads;
    int m_port;
    int m_listen_fd;

    static const int MAXFDS = 10000;

    std::unique_ptr<EventLoopThreadPool> m_evt_loop_th_pool_uptr;
    std::shared_ptr<Channel> m_accept_channel_sptr;
};