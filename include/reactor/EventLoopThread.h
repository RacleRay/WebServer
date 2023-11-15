#pragma once

#include "EventLoop.h"
#include "threads/Condition.h"
#include "threads/Mutex.h"
#include "threads/Thread.h"
#include "utils/noncopyable.h"


class EventLoopThread : Noncopyable {
public:
    EventLoopThread();
    ~EventLoopThread();

    EventLoop* start_loop();

private:
    void thread_func();

    EventLoop* m_loop{nullptr};
    Thread m_thread;
    mutable Mutex m_mutex{};
    Condition m_cond;

    bool is_exiting{false};
};