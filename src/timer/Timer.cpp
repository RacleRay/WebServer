#include "Timer.h"


// ==========================================================================
// TimerNode

TimerNode::TimerNode(std::shared_ptr<HttpData> request_data_sp, int timeout)
    :m_http_data_sp(request_data_sp) {
    struct timeval now;
    gettimeofday(&now, nullptr);
    m_expire_time = (now.tv_sec % 10'000 ) * 1000 + now.tv_usec / 1000 + timeout;
}

TimerNode::~TimerNode() {
    if (m_http_data_sp) {
        m_http_data_sp->handle_close();
    }
}


TimerNode::TimerNode(const TimerNode &tn) 
    : m_http_data_sp(tn.m_http_data_sp), m_expire_time(tn.m_expire_time) { }


void TimerNode::update(int timeout) {
    struct timeval now;
    gettimeofday(&now, nullptr);
    m_expire_time = (now.tv_sec % 10'000 ) * 1000 + now.tv_usec / 1000 + timeout;
}


bool TimerNode::is_valid() {
    struct timeval now;
    gettimeofday(&now, nullptr);
    size_t temp = (now.tv_sec % 10'000 ) * 1000 + now.tv_usec / 1000;
    if (temp < m_expire_time) {
        return true;
    }
    this->set_deleted();
    return false;
}


void TimerNode::clear_request() {
    m_http_data_sp.reset();
    this->set_deleted();
}


// ==========================================================================
// TimerManager

void TimerManager::add_timer(std::shared_ptr<HttpData> request_data_sp, int timeout) {
    TimerNodeSP new_node_sp(new TimerNode(request_data_sp, timeout));
    m_timer_queue.push(new_node_sp);
    request_data_sp->link_timer(new_node_sp);  // add link
}


void TimerManager::handle_expired_event() {
    while (!m_timer_queue.empty()) {
        TimerNodeSP top_node_sp = m_timer_queue.top();
        if (!top_node_sp->is_valid() || top_node_sp->is_deleted()) {
            m_timer_queue.pop();
        } else {
            break;
        }
    }
}