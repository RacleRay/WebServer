#include <functional>

#include "EventLoopThread.h"


EventLoopThread::EventLoopThread()
    : m_thread([this]() { thread_func(); }, "EventLoopThread"),
      m_cond(m_mutex) {}


EventLoopThread::~EventLoopThread() {
    is_exiting = true;
    if (m_loop) {
        m_loop->quit();
        m_thread.join();
    }
}


EventLoop* EventLoopThread::start_loop() {
    assert(!m_thread.started());

    m_thread.start();
    {
        MutexGuard lock(m_mutex);
        while (!m_loop) {
            m_cond.wait();
        }
    }

    return m_loop;
}

// 这里的 局部变量 evt_loop 由于 evt_loop.loop() 事件循环退出前不会返回，所以它不会被销毁
// m_loop 获取其地址，虽然很危险，但是是可以说得通的
void EventLoopThread::thread_func() {
    EventLoop evt_loop;

    {
        MutexGuard lock(m_mutex);
        m_loop = &evt_loop;
        m_cond.notify();
    }

    evt_loop.loop();

    m_loop = nullptr;
}