#ifndef HTTPCONNECTION_H
#define HTTPCONNECTION_H

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include "locker.h"

class http_conn
{
public:
    static const int FILENAME_LEN = 200;
    static const int READ_BUFFER_SIZE = 2048;
    static const int WRITE_BUFFER_SIZE = 1024;

    enum METHOD { GET = 0, POST, HEAD, PUT, DELETE, TRACE, OPTIONS, CONNECT, PATCH };
    // 解析客户请求时，状态机所处的状态
    enum CHECK_STATE { CHECK_STATE_REQUESTLINE = 0, CHECK_STATE_HEADER, CHECK_STATE_CONTENT };
    // 服务器处理HTTP请求的可能结果
    enum HTTP_CODE { NO_REQUEST, GET_REQUEST, BAD_REQUEST, NO_RESOURCE, FORBIDDEN_REQUEST, FILE_REQUEST, INTERNAL_ERROR, CLOSED_CONNECTION };
    // 行的读取状态
    enum LINE_STATUS { LINE_OK = 0, LINE_BAD, LINE_OPEN };

public:
    http_conn(){}
    ~http_conn(){}

public:
    void init( int sockfd, const sockaddr_in& addr );
    void close_conn( bool real_close = true );
    void process();
    bool read();  // 非阻塞读
    bool write();  // 非阻塞写

private:
    void init();
    HTTP_CODE process_read();  // 解析 HTTP 请求
    bool process_write( HTTP_CODE ret );  // 填充 HTTP 应答

    // 以下函数用于 process_read
    HTTP_CODE parse_request_line( char* text );
    HTTP_CODE parse_headers( char* text );
    HTTP_CODE parse_content( char* text );
    HTTP_CODE do_request();
    char* get_line() { return m_read_buf + m_start_line; }
    LINE_STATUS parse_line();

    // 以下函数用于 process_write
    void unmap();
    bool add_response( const char* format, ... );
    bool add_content( const char* content );
    bool add_status_line( int status, const char* title );
    bool add_headers( int content_length );
    bool add_content_length( int content_length );
    bool add_linger();
    bool add_blank_line();

public:
    // 所有 socket 上的事件，都被注册到同一个 epoll 内核事件表中
    static int m_epollfd;
    static int m_user_count;

private:
    int m_sockfd;   // connection fd
    sockaddr_in m_address;

    char m_read_buf[ READ_BUFFER_SIZE ];
    int m_read_idx;  // 读缓冲区中，已读入的客户端数据的最后一个字节的下一个字节
    int m_checked_idx;  // 正在被分析的字符位置
    int m_start_line;  // 正在被解析的行的起始位置
    char m_write_buf[ WRITE_BUFFER_SIZE ];
    int m_write_idx;   // 写缓冲区中待发送的字节数

    CHECK_STATE m_check_state;   // 主状态机所处的状态
    METHOD m_method;  // 请求方法

    char m_real_file[ FILENAME_LEN ];   // 客户请求的目标文件完整路径
    char* m_url;  // 目标文件名
    char* m_version;  // HTTP 版本号
    char* m_host;  // 主机名
    int m_content_length;  // HTTP 请求的消息体的长度
    bool m_linger;   // HTTP 请求是否要求保持连接

    char* m_file_address;   // 客户请求的目标文件被 mmap 到内存中的起始位置
    struct stat m_file_stat;  // 目标文件状态，判断文件是否存在、是否为目录、是否可读
    struct iovec m_iv[2];  // writev 执行写操作
    int m_iv_count;   // writev 被写的内存块数量
};

#endif
