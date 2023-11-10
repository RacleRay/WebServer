#pragma once

#include <memory>
#include <vector>

#include "EventLoopThread.h"
#include "logger/Logger.h"
#include "utils/noncopyable.h"


class EventLoopThreadPool : Noncopyable {
public:
    EventLoopThreadPool(EventLoop *base_loop, int num_threads);
    ~EventLoopThreadPool() {
        LOG << "EventLoopThreadPool dtor";
    }

    void start();
    EventLoop* get_next_loop();

private:
    EventLoop* m_base_loop;

    bool m_started{false};
    int m_next_idx{0};
    int m_num_threads;

    // EventLoop* 从 EventLoopThread 中获取
    std::vector<std::shared_ptr<EventLoopThread>> m_threads;
    std::vector<EventLoop*> m_evtloops;
};