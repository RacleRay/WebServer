#pragma once

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <sys/epoll.h>
#include <unistd.h>
#include <unordered_map>

#include "Timer.h"

class EventLoop;
class TimerNode;
class Channel;

enum class ProcessState {
    STATE_PARSE_URI = 1,
    STATE_PARSE_HEADERS,
    STATE_RECV_BODY,
    STATE_ANALYSIS,
    STATE_FINISH
};

enum class URIState {
    PARSE_URI_AGAIN = 1,
    PARSE_URI_ERROR,
    PARSE_URI_SUCCESS,
};

enum class HeaderState {
    PARSE_HEADER_SUCCESS = 1,
    PARSE_HEADER_AGAIN,
    PARSE_HEADER_ERROR
};

enum class AnalysisState { ANALYSIS_SUCCESS = 1, ANALYSIS_ERROR };

enum class ParseState {
    H_START = 0,
    H_KEY,
    H_COLON,
    H_SPACES_AFTER_COLON,
    H_VALUE,
    H_CR,
    H_LF,
    H_END_CR,
    H_END_LF
};

enum class ConnectionState { H_CONNECTED = 0, H_DISCONNECTING, H_DISCONNECTED };

enum class HttpMethod { METHOD_POST = 1, METHOD_GET, METHOD_HEAD };

enum class HttpVersion { HTTP_10 = 1, HTTP_11 };


class MimeType {
private:
    MimeType() = default;
    MimeType(const MimeType &m) = default;
    static const std::unordered_map<std::string, std::string> mime;

public:
    static std::string get_mime_type(const std::string &suffix);
};


class HttpData : public std::enable_shared_from_this<HttpData> {
public:
    HttpData(EventLoop *loop, int connfd);
    ~HttpData();
    
    void reset();

    void seperate_timer();
    void link_timer(std::shared_ptr<TimerNode> timer) {
        m_timer = timer;
    };

    std::shared_ptr<Channel> get_channel() { return m_channel; }

    EventLoop *get_loop() { return m_event_loop; }

    void handle_close();
    void add_new_event();

private:
    void handle_read();
    void handle_write();
    void handle_connect();
    void handle_error();

    URIState parse_URI();
    HeaderState parse_headers();
    AnalysisState analysis_request();

    std::weak_ptr<TimerNode> m_timer;
    std::shared_ptr<Channel> m_channel;

    EventLoop* m_event_loop;
};