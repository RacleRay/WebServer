#include <iostream>
#include <sys/epoll.h>
#include <sys/eventfd.h>

#include "EventLoop.h"
#include "Logger.h"
#include "Utils.h"

__thread EventLoop *thread_eventloop = nullptr;

int create_eventfd() {
    int evtfd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0) {
        LOG << "EventLoop::EventLoop() create eventfd failed";
        abort();
    }
    return evtfd;
}

EventLoop::EventLoop()
    : m_poller(new Epoll()), 
      m_wakeup_fd(create_eventfd()),
      m_thread_id(CurrentThread::get_tid()) 
{
    m_wakeup_channel = std::make_shared<Channel>(this, m_wakeup_fd);

    if (thread_eventloop == nullptr) {
        thread_eventloop = this;
    }

    m_wakeup_channel->set_events(EPOLLIN | EPOLLET);
    m_wakeup_channel->set_read_handler([this]() {handle_read();});
    m_wakeup_channel->set_conn_handler([this]() {handle_connection();});

    // no timer
    m_poller->epoll_add(m_wakeup_channel, 0);
}


EventLoop::~EventLoop() {
    close(m_wakeup_fd);
    m_wakeup_channel.reset();
    thread_eventloop = nullptr;
}


void EventLoop::handle_read() {
    uint64_t one;
    ssize_t n = readn(m_wakeup_fd, &one, sizeof(one));
    if (n != sizeof(one)) {
        LOG << "EventLoop::handleRead() reads " << n << " bytes instead of 8";
    }
    m_wakeup_channel->set_events(EPOLLIN | EPOLLET);
}


void EventLoop::handle_connection() {
    modify_poller(m_wakeup_channel, 0);
}


// wakeup 会唤醒所属的 EventLoop 线程中等待的 epoll_wait
void EventLoop::wakeup() {
    uint64_t one = 1;
    ssize_t n = writen(m_wakeup_fd, (char *)&one, sizeof(one));
    if (n != sizeof(one)) {
        LOG << "EventLoop::wakeup() writes " << n << " bytes instead of 8";
    }
}


/**
 * @brief 这里需要明确的点在于: 每个线程有一个EventLoop，但是当前运行的线程不一定是某个EventLoop
        所在的线程。每个EventLoop一般都阻塞在 epoll_wait 等待事件。
        如果当前运行线程对应于EventLoop所属线程，callback 会在线程上串行执行。
        如果当前运行线程不是EventLoop所属线程，callback 会被加入队列，并通过 eventfd 唤醒
        在 epoll_wait 上等待的线程。
 * 
 * @param cb 
 */
void EventLoop::run_in_loop(Functor&& cb) {
    if (is_in_loop_thread()) {
        cb();
    } else {
        queue_in_loop(std::move(cb));
    }
}


void EventLoop::queue_in_loop(Functor&& cb) {
    {
        MutexGuard lock(m_mutex);
        m_pending_functors.emplace_back(std::move(cb));
    }
    if (!is_in_loop_thread() || m_is_calling_pending_functors) {
        wakeup();
    }
}


void EventLoop::loop() {
    assert(!m_is_looping);
    assert(is_in_loop_thread());

    m_is_looping = true;
    m_is_quit = false;

    std::vector<std::shared_ptr<Channel>> active_channels;
    while (!m_is_quit) {
        active_channels.clear();
        active_channels = m_poller->get_active_events();
        
        // handle events 
        m_is_event_handling = true;
        for (auto& it: active_channels) {
            it->handle_events();
        }
        m_is_event_handling = false;

        // do pending callbacks
        do_pending_functors();

        // handle expired timers
        m_poller->handle_expired();
    }

    m_is_looping = false;
    m_is_quit = true;
}


void EventLoop::do_pending_functors() {
    std::vector<Functor> functors;
    m_is_calling_pending_functors = true;
    
    {
        MutexGuard lock(m_mutex);
        functors.swap(m_pending_functors);
    }

    for (auto& functor: functors) {
        functor();
    }

    m_is_calling_pending_functors = false;
}


void EventLoop::quit() {
    m_is_quit = true;
    if (!is_in_loop_thread()) {
        wakeup();
    }
}