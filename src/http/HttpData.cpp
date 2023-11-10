#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "Channel.h"
#include "EventLoop.h"
#include "HttpData.h"


// ONESHOT :  after an event is received for that file descriptor, it will be automatically removed from the  epoll  interest list.
constexpr uint32_t DEFAULT_EVENT = EPOLLIN | EPOLLET | EPOLLONESHOT;
constexpr int EXPIRED_TIME = 2000;  // ms
constexpr int KEEP_ALIVE_TIME = 5 * 60 * 1000;  // ms


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
   m_channel->set_owner_http(shared_from_this());
}


void HttpData::reset() {
    m_filename.clear();
    m_path.clear();
    m_read_pos = 0;

    m_headers.clear();
    m_process_state = ProcessState::STATE_PARSE_URI;
    m_parse_state = ParseState::H_START;

    detach_timer();
}


void HttpData::detach_timer() {
    // clear weak_ptr
    if (m_timer.lock()) {
        std::shared_ptr<TimerNode> tmp_timer(m_timer.lock());
        tmp_timer->invalidate_timer();
        m_timer.reset();
    }
}


void HttpData::add_new_event() {
    m_channel->set_events(DEFAULT_EVENT);
    m_event_loop->add_to_poller(m_channel, EXPIRED_TIME);
}


void HttpData::handle_read() {
    uint32_t &events = m_channel->get_events();

    // Reading Process
    bool nodata_flag = false;
    ssize_t read_num = read_utill_nodata(m_connfd, m_in_buf, nodata_flag);
    LOG << "Request: " << m_in_buf << "\n";
    if (m_connection_state == ConnectionState::H_DISCONNECTING) {
        m_in_buf.clear();
        goto out;
    }
    if (read_num < 0) {
        perror("read from client.");
        m_error = true;
        handle_error(m_connfd, 400, "Bad Request");
        goto out;
    }

    if (nodata_flag) {
        // 可能是数据未到达，或者对方异常关闭，都按照关闭处理
        m_connection_state = ConnectionState::H_DISCONNECTING;
        if (read_num == 0) {
            goto out;
        }
    }

    // Parse Request
    if (m_process_state == ProcessState::STATE_PARSE_URI) {
        URIState flag = this->parse_URI();
        if (flag == URIState::PARSE_URI_AGAIN) {
            goto out;
        }
        if (flag == URIState::PARSE_URI_ERROR) {
            perror("parse_URI error");
            LOG << "FD = " << m_connfd << ", " << m_in_buf << "*** Error. \n";
            m_in_buf.clear();
            m_error = true;
            handle_error(m_connfd, 400, "Bad Request");
            goto out;
        }
        m_process_state = ProcessState::STATE_PARSE_HEADERS;
    }

    if (m_process_state == ProcessState::STATE_PARSE_HEADERS) {
        HeaderState flag = this->parse_headers();
        if (flag == HeaderState::PARSE_HEADER_AGAIN) {
            goto out;
        }
        if (flag == HeaderState::PARSE_HEADER_ERROR) {
            perror("parse_headers error");
            LOG << "FD = " << m_connfd << ", " << m_in_buf << "*** Error. \n";
            m_in_buf.clear();
            handle_error(m_connfd, 400, "Bad Request");
            goto out;
        }

        if (m_method == HttpMethod::METHOD_GET) {
            m_process_state = ProcessState::STATE_RECV_BODY;
        } else {
            m_process_state = ProcessState::STATE_ANALYSIS;
        }
    }

    if (m_process_state == ProcessState::STATE_RECV_BODY) {
        int content_length = -1;
        if (m_headers.find("Content-Length") != m_headers.end()) {
            content_length = std::stoi(m_headers["Content-Length"]);
        } else {
            m_error = true;
            handle_error(m_connfd, 400, "Bad Request: Content-Length not find");
            goto out; 
        }
        if (static_cast<int>(m_in_buf.size()) < content_length) {
            goto out;
        }
        m_process_state = ProcessState::STATE_ANALYSIS;
    }    

    if (m_process_state == ProcessState::STATE_ANALYSIS) {
        AnalysisState flag = this->analysis_request();
        if (flag == AnalysisState::ANALYSIS_SUCCESS) {
            m_process_state = ProcessState::STATE_FINISH;
            goto out;
        } else {
            m_error = true;
            goto out;
        }
    }

out:
    if (!m_error) {
        if (!m_out_buf.empty()) {
            handle_write();
        }

        // 可能在 handle_write 改变了 m_error
        if (!m_error && m_process_state == ProcessState::STATE_FINISH) {
            this->reset();
        // 注册 EPOLLIN
        } else if (!m_error && m_connection_state != ConnectionState::H_DISCONNECTED) {
            events |= EPOLLIN;
        }
    }
}


void HttpData::handle_write() {
    if (!m_error && m_connection_state != ConnectionState::H_DISCONNECTED) {
        uint32_t &events = m_channel->get_events();
        if (writen(m_connfd, m_out_buf) < 0) {
            perror("writen to client.");
            events = 0;
            m_error = true;
        }
        if (!m_out_buf.empty()) {
            events |= EPOLLOUT;
        }
    }
}


void HttpData::handle_connect() {
    int timeout = EXPIRED_TIME;
    detach_timer();

    uint32_t &events = m_channel->get_events();
    if (!m_error && m_connection_state == ConnectionState::H_CONNECTED) {
        if (events != 0) {
            if ((events & EPOLLIN) && (events & EPOLLOUT)) {
                events = static_cast<uint32_t>(0);
                events |= EPOLLOUT;
            }
            // NOTE: keep alive setting
            if (m_keep_alive) {
                timeout = KEEP_ALIVE_TIME;
            }
            events |= EPOLLET;
            m_event_loop->modify_poller(m_channel, timeout);
        } else if (m_keep_alive) {
            events |= (EPOLLIN | EPOLLET);
            timeout = KEEP_ALIVE_TIME;
            m_event_loop->modify_poller(m_channel, timeout);
        } else {
            events |= (EPOLLIN | EPOLLET);
            m_event_loop->modify_poller(m_channel, timeout);
        }
    } else if (!m_error && m_connection_state == ConnectionState::H_DISCONNECTING 
               && (events & EPOLLOUT)) {
        events = (EPOLLOUT | EPOLLET);  // 关闭中，发送剩余数据
    } else {
        m_event_loop->run_in_loop([this]() { handle_close(); });
    }
}


void HttpData::handle_close() {
    m_connection_state = ConnectionState::H_DISCONNECTED;
    m_event_loop->remove_from_poller(m_channel);
}


URIState HttpData::parse_URI() {
    std::string& content = m_in_buf;

    // 有完整请求行(第一行)之后再解析
    size_t pos = content.find('\r', m_read_pos);
    if (pos == content.npos) {
        return URIState::PARSE_URI_AGAIN;
    }

    // 截取出请求行
    std::string request_line = content.substr(0, pos);
    if (content.size() > pos + 1) {
        content = content.substr(pos + 1);
    } else {
        content.clear();
    }

    size_t get_pos = request_line.find("GET");
    size_t post_pos = request_line.find("POST");
    size_t head_pos = request_line.find("HEAD");

    if (get_pos != std::string::npos) {
        pos = get_pos;
        m_method = HttpMethod::METHOD_GET;
    } else if (post_pos != std::string::npos) {
        pos = post_pos;
        m_method = HttpMethod::METHOD_POST;
    } else if (head_pos != std::string::npos) {
        pos = head_pos;
        m_method = HttpMethod::METHOD_HEAD;
    } else {
        return URIState::PARSE_URI_ERROR;
    }

    // 默认 index.html
    pos = request_line.find('/', pos);
    if (pos < 0) {
        m_filename = "index.html";
        m_http_version = HttpVersion::HTTP_11;
        return URIState::PARSE_URI_SUCCESS;
    }

    // 获取文件名
    size_t file_name_end = request_line.find(' ', pos);
    if (file_name_end == std::string::npos) {
        return URIState::PARSE_URI_ERROR;
    } 
    if (file_name_end - pos > 1) {
        m_filename = request_line.substr(pos + 1, file_name_end - pos - 1);
        size_t qmark_pos = m_filename.find('?');
        if (qmark_pos != std::string::npos) {
            m_filename = m_filename.substr(0, qmark_pos);
        }
    } else {
        m_filename = "index.html";
    }
    pos = file_name_end;

    // HTTP 版本解析
    pos = request_line.find('/', pos);
    if (pos == std::string::npos) {
        return URIState::PARSE_URI_ERROR;
    }

    if (request_line.size() - pos <= 3) {
        return URIState::PARSE_URI_ERROR;
    }

    std::string ver_str = request_line.substr(pos + 1, 3);
    if (ver_str == "1.0") {
        m_http_version = HttpVersion::HTTP_10;
    } else if (ver_str == "1.1") {
        m_http_version = HttpVersion::HTTP_11;
    } else {
        return URIState::PARSE_URI_ERROR;
    }

    return URIState::PARSE_URI_SUCCESS;
}


HeaderState HttpData::parse_headers() {
    std::string& content = m_in_buf;
    size_t key_start = 0, key_end = 0, value_start = 0, value_end = 0;
    size_t current_pos = 0;
    bool is_finished = false;

    // 有限状态机解析 HTTP 请求头
    size_t i = 0;
    for ( ; i < content.size() && !is_finished; ++i) {
        switch (m_parse_state) {
            case ParseState::H_START: {
                if (content[i] == '\r' || content[i] == '\n') {
                    break;
                }
                m_parse_state = ParseState::H_KEY;
                key_start = i;
                current_pos = i;
                break;
            }

            case ParseState::H_KEY: {
                if (content[i] == ':') {
                    key_end = i;
                    if (key_end - key_start <= 0) {
                        return HeaderState::PARSE_HEADER_ERROR;
                    }
                    m_parse_state = ParseState::H_COLON;
                } else if (content[i] == '\r' || content[i] == '\n') {
                    return HeaderState::PARSE_HEADER_ERROR;
                }
                break;
            }
            
            case ParseState::H_COLON: {
                if (content[i] == ' ') {
                    m_parse_state = ParseState::H_SPACES_AFTER_COLON;
                } else {
                    return HeaderState::PARSE_HEADER_ERROR;
                }
                break;
            }

            case ParseState::H_VALUE: {
                if (content[i] == '\r') {
                    m_parse_state = ParseState::H_CR;
                    value_end = i;
                    if (value_end - value_start <= 0) {
                        return HeaderState::PARSE_HEADER_ERROR;
                    }
                } else if (i - value_start > 255) {
                    return HeaderState::PARSE_HEADER_ERROR;
                }
                break;                
            }

            case ParseState::H_SPACES_AFTER_COLON: {
                m_parse_state = ParseState::H_VALUE;
                value_start = i;
                break;
            }
            // \n
            case ParseState::H_CR: {
                if (content[i] == '\n') {
                    m_parse_state = ParseState::H_LF;
                    std::string key(content.begin() + key_start, content.begin() + key_end);
                    std::string value(content.begin() + value_start, content.begin() + value_end);
                    m_headers[key] = value;
                    current_pos = i;
                } else {
                    return HeaderState::PARSE_HEADER_ERROR;
                }
                break;
            }

            case ParseState::H_LF: {
                // 此时读取到 结束行 \r\n
                if (content[i] == '\r') {
                    m_parse_state = ParseState::H_END_CR;
                } else {
                    key_start = i;
                    m_parse_state = ParseState::H_KEY;
                }
                break;
            }

            case ParseState::H_END_CR: {
                if (content[i] == '\n') {
                    m_parse_state = ParseState::H_END_LF;
                } else {
                    return HeaderState::PARSE_HEADER_ERROR;
                }
                break;
            }

            case ParseState::H_END_LF: {
                is_finished = true;
                key_start = i;
                current_pos = i;
                break;
            }
        }
    }

    if (m_parse_state == ParseState::H_END_LF) {
        content = content.substr(i);
        return HeaderState::PARSE_HEADER_SUCCESS;
    }

    content = content.substr(current_pos);
    return HeaderState::PARSE_HEADER_AGAIN;
}


AnalysisState HttpData::analysis_request() {
    if (m_method == HttpMethod::METHOD_POST) {
        m_out_buf.clear();
        handle_error(m_connfd, 403, "Forbidden Request.");
        return AnalysisState::ANALYSIS_ERROR;
    }
    if (m_method == HttpMethod::METHOD_GET || m_method == HttpMethod::METHOD_HEAD) {
        // response header
        std::string header;
        header += "HTTP/1.1 200 OK\r\n";
        if (m_headers.find("Connection") != m_headers.end() 
            && (m_headers["Connection"] == "keep-alive" ||
                m_headers["Connection"] == "Keep-Alive")) {
            m_keep_alive = true;
            header += "Connection: keep-alive\r\nKeep-Alive: timeout=" +
                std::to_string(KEEP_ALIVE_TIME) + "\r\n";
        }

        // find filetype
        size_t dot_pos = m_filename.find('.');
        std::string filetype;
        if (dot_pos < 0) {
            filetype = MimeType::get_mime_type("default");
        } else {
            filetype = MimeType::get_mime_type(m_filename.substr(dot_pos));
        }

        // if filename for test
        if (m_filename == "hellotest") {
            m_out_buf = "HTTP/1.1 200 OK\r\nContent-type: text/plain\r\n\r\nHello World";
            return AnalysisState::ANALYSIS_SUCCESS;
        }

        // find file
        struct stat sbuf;
        if (stat(m_filename.c_str(), &sbuf) < 0) {
            header.clear();
            handle_error(m_connfd, 404, "Not Found!");
            return AnalysisState::ANALYSIS_ERROR;
        }

        // header information for response
        header += "Content-Type: " + filetype + "\r\n";
        header += "Content-Length: " + std::to_string(sbuf.st_size) + "\r\n";
        header += "Server: Static Web Server\r\n";
        header += "\r\n";

        m_out_buf += header;

        if (m_method == HttpMethod::METHOD_HEAD) {
            return AnalysisState::ANALYSIS_SUCCESS;
        }

        // prepare file contents to send
        int file_fd = open(m_filename.c_str(), O_RDONLY, 0);
        if (file_fd < 0) {
            m_out_buf.clear();
            handle_error(m_connfd, 404, "Not Found!");
            return AnalysisState::ANALYSIS_ERROR;
        }

        // mmap file
        void *mmap_file = mmap(nullptr, sbuf.st_size, PROT_READ, MAP_PRIVATE, file_fd, 0);
        close(file_fd);
        if (mmap_file == MAP_FAILED) {
            munmap(mmap_file, sbuf.st_size);
            m_out_buf.clear();
            handle_error(m_connfd, 404, "Not Found!");
            return AnalysisState::ANALYSIS_ERROR;
        }

        char* file_addr = static_cast<char*>(mmap_file);
        // string buf
        m_out_buf += std::string(file_addr, sbuf.st_size);
        munmap(file_addr, sbuf.st_size);
        return AnalysisState::ANALYSIS_SUCCESS;
    }

    return AnalysisState::ANALYSIS_ERROR;
}