#include <arpa/inet.h>
#include <functional>
#include <netinet/in.h>
#include <sys/socket.h>

#include "Server.h"
#include "Utils.h"


Server::Server(EventLoop* loop, int num_threads, int port)
    : m_main_loop(loop), m_num_threads(num_threads), m_port(port) {
    m_evt_loop_th_pool_uptr = std::make_unique<EventLoopThreadPool>(m_main_loop, m_num_threads);
    m_accept_channel_sptr = std::make_shared<Channel>(m_main_loop);
    m_listen_fd = socket_bind_listen(m_port);

    m_accept_channel_sptr->set_fd(m_listen_fd);
    
    handle_sigpipe();  // ignore

    if (set_socket_nonblock(m_listen_fd) < 0) {
        perror("set_socket_nonblock failed.");
        abort();
    }
}


void Server::start() {
    // start thread pool
    m_evt_loop_th_pool_uptr->start();

    // m_events 初始值
    m_accept_channel_sptr->set_events(EPOLLIN | EPOLLET);
    // accept_channel 的事件回调函数
    m_accept_channel_sptr->set_read_handler([this]() {handle_available_connfd();});
    m_accept_channel_sptr->set_conn_handler([this]() {handle_connect();});
    
    // 添加到事件循环，进行监控
    m_main_loop->add_to_poller(m_accept_channel_sptr, 0);
}


void Server::handle_available_connfd() {
    struct sockaddr_in client_addr;
    bzero(&client_addr, sizeof(client_addr));
    socklen_t client_addr_len = sizeof(client_addr);

    int accept_fd = 0;
    while ((accept_fd = accept(m_listen_fd, (struct sockaddr*)&client_addr, &client_addr_len)) > 0) {
        // active_loop 属于另一个线程，被好 wakeup 后会处理 queue 中的 callback
        EventLoop* active_loop = m_evt_loop_th_pool_uptr->get_next_loop();

        LOG << "New connection, fd = " << accept_fd << ", ip = " 
            << inet_ntoa(client_addr.sin_addr)
            << ", port = " << ntohs(client_addr.sin_port);
        
        if (accept_fd >= MAXFDS) {
            close(accept_fd);
            continue;
        }

        if (set_socket_nonblock(accept_fd) < 0) {
            LOG << "set_socket_nonblock failed.";
            return;
        }

        set_socket_nodelay(accept_fd);

        // request_httpdata 对应某个 active_loop
        // 向 active_loop 中注册 新的事件 ，默认为 EPOLLIN | EPOLLET | EPOLLONESHOT
        std::shared_ptr<HttpData> request_httpdata(new HttpData(active_loop, accept_fd));
        active_loop->queue_in_loop([request_httpdata]() {request_httpdata->add_new_event();});
    }
}


void Server::handle_connect() {
    // 实际上 m_main_loop 的 accept_channel 的事件
    // is_events_sameas_last 都是真的
    m_main_loop->modify_poller(m_accept_channel_sptr, 0);   
}
