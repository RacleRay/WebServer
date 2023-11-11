#include <arpa/inet.h>
#include <functional>
#include <netinet/in.h>
#include <sys/socket.h>

#include "Server.h"
#include "Utils.h"

#include "Debug.h"


Server::Server(EventLoop* loop, int num_threads, int port)
    : m_main_loop(loop), m_num_threads(num_threads), m_port(port)
{
    m_evt_loop_th_pool = std::make_unique<EventLoopThreadPool>(m_main_loop, m_num_threads);
    m_accept_channel = std::make_shared<Channel>(m_main_loop);
    m_listen_fd = socket_bind_listen(m_port);
    
    handle_sigpipe();  // ignore

    if (set_socket_nonblock(m_listen_fd) < 0) {
        perror("set_socket_nonblock failed.");
        abort();
    }

    m_accept_channel->set_fd(m_listen_fd);

    PRINT("Listening on port: " << m_port);
}


void Server::start() {
    // start thread pool
    m_evt_loop_th_pool->start();

    // 主线程作为监听连接请求的线程，设置 m_events 初始值
    m_accept_channel->set_events(EPOLLIN | EPOLLET);
    // accept_channel 的事件回调函数
    m_accept_channel->set_read_handler([this]() {this->handle_available_connfd();});
    m_accept_channel->set_conn_handler([this]() {this->handle_connect();});;
    
    // 添加到事件循环，进行监控
    m_main_loop->add_to_poller(m_accept_channel, 0);
    
    m_started = true;
}


void Server::handle_available_connfd() {
    struct sockaddr_in client_addr;
    bzero(&client_addr, sizeof(client_addr));
    socklen_t client_addr_len = sizeof(client_addr);

    int conn_fd = 0;
    // 由于是多线程，如果多个连接就绪，边缘触犯只会触发一次，accept只处理一个连接，
    // TCP 就绪队列中的连接得不到处理，所以使用 while
    while ((conn_fd = accept(m_listen_fd, (struct sockaddr*)&client_addr, 
                            &client_addr_len)) > 0) 
    {
        // active_loop 属于另一个线程，被好 wakeup 后会处理 queue 中的 callback
        EventLoop* active_loop = m_evt_loop_th_pool->get_next_loop();

        LOG << "New connection, fd = " << conn_fd << ", ip = " 
            << inet_ntoa(client_addr.sin_addr)
            << ", port = " << ntohs(client_addr.sin_port);
        
        PRINT("New connection, fd = " << conn_fd << ", ip = " << inet_ntoa(client_addr.sin_addr));

        if (conn_fd >= MAXFDS) {
            close(conn_fd);
            continue;
        }

        if (set_socket_nonblock(conn_fd) < 0) {
            LOG << "set_socket_nonblock failed.";
            return;
        }

        set_socket_nodelay(conn_fd);

        // request_httpdata 对应某个 active_loop
        // 向 active_loop 中注册 新的事件 ，默认为 EPOLLIN | EPOLLET | EPOLLONESHOT
        std::shared_ptr<HttpData> request_httpdata(new HttpData(active_loop, conn_fd));
        request_httpdata->get_channel()->set_owner_http(request_httpdata);
        active_loop->queue_in_loop([request_httpdata]() {request_httpdata->add_new_event();});
    }

    // 这一步非常非常关键！BUG制造者！
    // 由于在 Epoll::collect_active_channels 中重置了 events 为 0
    // 需要重新设置注册事件为 EPOLLIN | EPOLLET，以继续监听网络连接
    m_accept_channel->set_events(EPOLLIN | EPOLLET);
}


void Server::handle_connect() {
    // 实际上 m_main_loop 的 accept_channel 的事件
    // is_events_sameas_last 都是真的
    PRINT("Server handle_connect on fd " << m_listen_fd);
    m_main_loop->modify_poller(m_accept_channel, 0);   
}
