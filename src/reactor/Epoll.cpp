#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <deque>
#include <queue>
#include <iostream>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>

#include "Epoll.h"
#include "Logger.h"
#include "Utils.h"

#include "Debug.h"


const int EVENTS_NUM = 4096;
const int EPOLLWAIT_TIME = 10'000;


Epoll::Epoll() : m_epoll_fd(epoll_create1(EPOLL_CLOEXEC)), m_events_buf(EVENTS_NUM) {
    assert(m_epoll_fd > 0);
}


// 每个一个连接描述符，对应一个 Channel，描述符由 epoll 监控
// 每个 Channel 对应一个 HttpData 对象，Channel 作为 HttpData 的操作中间工具类
// Channel 负责接口，HttpData 负责实际的数据处理逻辑
void Epoll::epoll_add(std::shared_ptr<Channel> req_channel, int timeout) {
    int fd = req_channel->get_fd();
    if (timeout > 0) {
        add_timer(req_channel, timeout);
        m_fd2httpdata[fd] = req_channel->get_owner_http();
    }

    struct epoll_event event;
    event.data.fd = fd;
    event.events = req_channel->get_events();  // 注册 m_events 到epoll中
    // req_channel->updata_last_events();
    req_channel->compare_and_updata_last_evt();

    m_fd2channels[fd] = req_channel;
    int ret = epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, fd, &event);
    if (ret < 0) {
        perror("epoll_add failed.");
        m_fd2channels[fd].reset();
    }
}


void Epoll::epoll_mod(std::shared_ptr<Channel> req_channel, int timeout) {
    if (timeout > 0) {
        add_timer(req_channel, timeout);
    }

    int fd = req_channel->get_fd();
    // events 会在 collect_active_channels 中更新，表示 Epoll 已经处理了事件
    if (!req_channel->compare_and_updata_last_evt()) {
        // req_channel->updata_last_events();
        struct epoll_event event;
        event.data.fd = fd;
        event.events = req_channel->get_events();
        int ret = epoll_ctl(m_epoll_fd, EPOLL_CTL_MOD, fd, &event);
        if (ret < 0) {
           perror("epoll_mod failed.");
           m_fd2channels[fd].reset();
        }
    }
}


void Epoll::epoll_del(std::shared_ptr<Channel> req_channel) {
    int fd = req_channel->get_fd();

    struct epoll_event event;
    event.data.fd = fd;
    event.events = req_channel->get_lastevents();  // 已将 m_events 保存到 m_lastevents
    int ret = epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, fd, &event);
    if (ret < 0) {
        perror("epoll_del failed.");
    }
    m_fd2channels[fd].reset();
    m_fd2httpdata[fd].reset();
}


std::vector<std::shared_ptr<Channel>> Epoll::get_active_events() {
    while (true) {
        PRINT("epoll_wait on " << m_epoll_fd << ". " << "epoll buf size " << m_events_buf.size());
        int event_count = epoll_wait(m_epoll_fd, &*m_events_buf.begin(), m_events_buf.size(), EPOLLWAIT_TIME);
        if (event_count < 0) {
            perror("epoll_wait failed.");
        }
        std::vector<std::shared_ptr<Channel>> active_channels = collect_active_channels(event_count);
        if (!active_channels.empty()) {
            return active_channels;
        }
    }
}


std::vector<std::shared_ptr<Channel>> Epoll::collect_active_channels(int event_count) {
    std::vector<std::shared_ptr<Channel>> active_channels;
    for (int i = 0; i < event_count; ++i) {
        int fd = m_events_buf[i].data.fd;
        std::shared_ptr<Channel> req_channel = m_fd2channels[fd];
        PRINT("active epoll fd: " << fd);
        PRINT("active channel event is : " << req_channel->get_events());
        if (!req_channel) {
            LOG << "Epoll::get_active_channels: shared_ptr req_channel is nullptr.";
        } else {
            // epoll wait 活跃事件
            req_channel->set_revents(m_events_buf[i].events);
            req_channel->set_events(0);   // 已响应，重置
            active_channels.push_back(req_channel);
        }
    }
    return active_channels;
}


void Epoll::add_timer(std::shared_ptr<Channel> req_channel, int timeout) {
    std::shared_ptr<HttpData> http_data = req_channel->get_owner_http();
    if (http_data) {
        m_timer_manager.add_timer(http_data, timeout);
    } else {
        LOG << "Epoll::add_timer: shared_ptr http_data is nullptr.";
    }
}


void Epoll::handle_expired() {
    m_timer_manager.handle_expired_event();
}