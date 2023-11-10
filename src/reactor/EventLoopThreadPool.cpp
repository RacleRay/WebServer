#include "EventLoopThreadPool.h"


EventLoopThreadPool::EventLoopThreadPool(EventLoop *base_loop, int num_threads) 
    : m_base_loop(base_loop), m_num_threads(num_threads)  {
    if (num_threads <= 0) {
        LOG << "EventLoopThreadPool init with num_threads <= 0";
        abort();
    }
}


// 开启 num_threads 的线程，每个运行一个 EventLoop.
void EventLoopThreadPool::start() {
    m_base_loop->assert_in_loop_thread();
    m_started = true;

    for (int i = 0; i < m_num_threads; i++) {
        std::shared_ptr<EventLoopThread> t(new EventLoopThread());
        m_threads.push_back(t);
        m_evtloops.push_back(t->start_loop());
    }
}


// 从 EventLoop 线程池中，循环取出 EventLoop
// 简单的 Round Robin
EventLoop* EventLoopThreadPool::get_next_loop() {
    m_base_loop->assert_in_loop_thread();
    assert(m_started);

    EventLoop* loop = m_base_loop;
    if (!m_evtloops.empty()) {
        loop = m_evtloops[m_next_idx];
        m_next_idx = (m_next_idx + 1) % m_num_threads;
    }

    return loop;
}