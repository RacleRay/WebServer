#pragma once

#include <functional>
#include <iostream>
#include <memory>
#include <vector>

#include "Channel.h"
#include "Epoll.h"
#include "logger/Logger.h"
#include "threads/CurrentThread.h"
#include "threads/Thread.h"
#include "utils/Utils.h"

// One loop per thread

class EventLoop {
public:
    using Functor = std::function<void()>;

    EventLoop();
    ~EventLoop();

    void loop();
    void quit();

    void run_in_loop(Functor&& cb);
    void queue_in_loop(Functor&& cb);

    [[nodiscard]] bool is_in_loop_thread() const { return (m_thread_id == CurrentThread::get_tid()); }
    void assert_in_loop_thread() const {
        assert(is_in_loop_thread());
    }

    void shutdown(std::shared_ptr<Channel> channel) {
        shutdown_WR(channel->get_fd());
    };

    void remove_from_poller(std::shared_ptr<Channel> channel) {
        m_poller->epoll_del(channel);
    }

    void modify_poller(std::shared_ptr<Channel> channel, int timeout) {
        m_poller->epoll_mod(channel, timeout);
    }

    void add_to_poller(std::shared_ptr<Channel> channel, int timeout) {
        m_poller->epoll_add(channel, timeout);
    }

private:
    bool m_is_looping{false};
    bool m_is_quit{false};
    bool m_is_calling_pending_functors{false};
    bool m_is_event_handling{false};

    int m_wakeup_fd;

    Mutex m_mutex;
    
    const pid_t m_thread_id;
    
    std::shared_ptr<Epoll> m_poller;
    std::shared_ptr<Channel> m_wakeup_channel;

    std::vector<Functor> m_pending_functors;

    void wakeup();
    void handle_read();
    void do_pending_functors();
    void handle_connection();
};
