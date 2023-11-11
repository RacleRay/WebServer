#include "Channel.h"

#include "Debug.h"


// 处理 epoll_wait 返回的活跃事件
void Channel::handle_revents() {
    // events 需要在处理之后，再次 set_events
    m_events = 0;

    // hang-up event: when the remote end of a connection is closed or when an error is encountered
    if ((m_revents & EPOLLHUP) && !(m_revents & EPOLLIN)) {  // 连接关闭或错误
        m_events = 0;
        return;
    }

    if (m_revents & EPOLLERR) {
        handle_error();
        m_events = 0;
        return;
    }

    // EPOLLPRI: urgent data (OOB) is available for reading 
    // EPOLLRDHUP: remote end closed the connection or shutdown the writing end of the socket
    if (m_revents & (EPOLLIN | EPOLLPRI | EPOLLRDHUP)) {  // 可读
        // PRINT("epoll event is " << m_revents);
        // PRINT("handle read");
        handle_read();
    }

    if (m_revents & EPOLLOUT) {  // 可写
        handle_write();
    }

    handle_connect();   // 处理连接响应或者wakeup请求
}
