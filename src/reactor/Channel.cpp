#include "Channel.h"


void Channel::handle_events() {
    // hang-up event: when the remote end of a connection is closed or when an error is encountered
    if ((m_revents & EPOLLHUP) && !(m_revents & EPOLLIN)) {
        m_events = 0;
        return;
    }

    if (m_revents & EPOLLERR) {
        m_events = 0;
        handle_error();
        return;
    }

    // EPOLLPRI: urgent data (OOB) is available for reading 
    // EPOLLRDHUP: remote end closed the connection or shutdown the writing end of the socket
    if (m_revents & (EPOLLIN | EPOLLPRI | EPOLLRDHUP)) {
        handle_read();
    }

    if (m_revents & EPOLLOUT) {
        handle_write();
    }

    handle_connect();
}