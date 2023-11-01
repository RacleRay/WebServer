#pragma once

#include <functional>
#include <iostream>
#include <memory>
#include <vector>

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

    void shutdown();


private:
    bool m_looping;
    bool m_quit;
    bool m_calling_pending_functors;
    bool m_event_handling;

    int m_wakeup_fd;

    Mutex m_mutex;
    
    const pid_t m_thread_id;
    

};
