
#include "Channel.h"
#include "EventLoop.h"
#include "HttpData.h"



// ==========================================================================
// MimeType

const std::unordered_map<std::string, std::string> mime{
    {".html", "text/html"},
    {".css", "text/css"},
    {".js", "application/javascript"},
    {".png", "image/png"},
    {".jpg", "image/jpeg"},
    {".gif", "image/gif"},
    {".ico", "image/x-icon"},
    {".svg", "image/svg+xml"},
    {".json", "application/json"},
    {".pdf", "application/pdf"},
    {".zip", "application/zip"},
    {".mp4", "video/mp4"},
    {".mp3", "audio/mp3"},
    {".txt", "text/plain"},
    {".xml", "text/xml"},
    {".htm", "text/html"},
    {".c", "text/plain"},
    {".txt", "text/plain"},
    {"default", "text/html"}
};


std::string MimeType::get_mime_type(const std::string &suffix) {
    if (mime.find(suffix) != mime.end()) {
        return mime.at(suffix);
    }
    return mime.at("default");
}


// ==========================================================================
// HttpData

HttpData::HttpData(EventLoop *loop, int connfd)
    : m_event_loop(loop), m_connfd(connfd), 
      m_channel(new Channel(loop, connfd)) {
   m_channel->set_read_handler([this](){handle_read();});
   m_channel->set_write_handler([this](){handle_write();});
   m_channel->set_conn_handler([this](){handle_connect();});
}


void HttpData::reset() {
    m_filename.clear();
    m_path.clear();
    m_read_pos = 0;

    m_headers.clear();
    m_process_state = ProcessState::STATE_PARSE_URI;
    m_parse_state = ParseState::H_START;

    // clear weak_ptr
    if (m_timer.lock()) {
        std::shared_ptr<TimerNode> tmp_timer(m_timer.lock());
        tmp_timer->clear_request();
        m_timer.reset();
    }
}


