
#include <cerrno>
#include <csignal>
#include <cstring>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>

#include "Utils.h"


constexpr int MAX_BUF = 4096;


/**
 * @brief fd 是阻塞模式还是非阻塞模式，错误码的含义不同。需要分开分析。 

read:
 非阻塞模式：如果文件描述符处于非阻塞模式，而且当前没有可用的数据可供读取，read系统调用将立即返回并设置EAGAIN错误码。
    这样可以避免阻塞并允许程序执行其他任务。
 阻塞模式超时：在某些操作系统中，可以设置文件描述符的超时属性。
    如果在阻塞模式下进行读取，并且超过了指定的超时时间，read系统调用将返回并设置EAGAIN错误码。

write:
 非阻塞模式：如果文件描述符处于非阻塞模式，而且当前无法立即写入所有请求的数据，write系统调用将立即返回并设置EAGAIN错误码。
    这样可以避免阻塞并允许程序执行其他任务。
 写缓冲区已满：当写入的数据量超过文件描述符关联的写缓冲区的容量时，write系统调用可能会返回EAGAIN错误码。
    这意味着操作系统当前无法立即接受更多的数据，需要等待写缓冲区中的数据被写入磁盘或发送到目标。
 * 
 */


// read n bytes from fd to buff
ssize_t readn(int fd, void *buff, size_t n) {
    size_t nleft = n;
    ssize_t onetime_read = 0;
    ssize_t total_read = 0;

    char* ptr = (char *)buff;
    while (nleft > 0) {
        onetime_read = read(fd, ptr, nleft);
        if (onetime_read < 0) {
            if (errno == EINTR) {  // have to read from beginning again
                onetime_read = 0;
                continue;
            }
            if (errno == EAGAIN) {  // have to read from current position again
                return total_read;
            } 
            perror("readn failed.");
            return -1;
        } 
        if (onetime_read == 0) {
            break;
        }

        total_read += onetime_read;
        nleft -= onetime_read;
        ptr += onetime_read;
    }

    return total_read;
}

// read data from fd to buff until no more data or timeout(if set)
ssize_t read_utill_nodata(int fd, std::string &buff, bool &nodata) {
    ssize_t nread = 0;
    ssize_t total_read = 0;

    while (true) {
        char temp[MAX_BUF];
        nread = read(fd, temp, MAX_BUF);
        if (nread < 0) {
            if (errno == EINTR) {
                continue;  // read from beginning again
            }
            if (errno == EAGAIN) {
                return total_read;  // read from current position again
            } 
            perror("read_till_nodata failed.");
            return -1;
        } 
        if (nread == 0) {
            nodata = true;
            break;
        }

        total_read += nread;
        buff.append(temp, temp + nread);
    }

    return total_read;
}


// same as read_utill_nodata(int fd, std::string &buff, bool &nodata), but without nodata flag
ssize_t read_utill_nodata_nflag(int fd, std::string &buff) {
    ssize_t nread = 0;
    ssize_t total_read = 0;

    while (true) {
        char temp[MAX_BUF];
        nread = read(fd, temp, MAX_BUF);
        if (nread < 0) {
            if (errno == EINTR) {
                continue;  // read from beginning again
            }
            if (errno == EAGAIN) {
                return total_read;  // read from current position again
            } 
            perror("readn failed.");
            return -1;
        } 
        if (nread == 0) {
            break;
        }

        total_read += nread;
        buff.append(temp, temp + nread);
    }

    return total_read;
}


// write n bytes from buff to fd
ssize_t writen(int fd, void *buff, size_t n) {
    size_t nleft = n;
    ssize_t onetime_write = 0;
    ssize_t total_write = 0;
    char* ptr = (char *)buff;

    while (nleft > 0) {
        onetime_write = write(fd, ptr, nleft);
        if (onetime_write < 0) {
            if (errno == EINTR) {
                onetime_write = 0;
                continue;
            }
            if (errno == EAGAIN) {
                return total_write;
            }
            perror("writen failed.");
            return -1;
        }

        total_write += onetime_write;
        nleft -= onetime_write;
        ptr += onetime_write;
    }

    return total_write;
}


// write data from buff to fd.
ssize_t writen(int fd, std::string &buff) {
    size_t nleft = buff.size();
    ssize_t onetime_write = 0;
    ssize_t total_write = 0;
    const char *ptr = buff.c_str();

    while (nleft > 0) {
        onetime_write = write(fd, ptr, nleft);
        if (onetime_write < 0) {
            if (errno == EINTR) {
                onetime_write = 0;
                continue;
            }
            if (errno == EAGAIN) {
                break;   // different from above implementation
            }
            perror("writen failed.");
            return -1;
        }

        total_write += onetime_write;
        nleft -= onetime_write;
        ptr += onetime_write;
    }

    if (total_write == static_cast<ssize_t>(buff.size())) {
        buff.clear();  // totally writen to fd, clear buffer
    } else {
        buff = buff.substr(total_write);  // partially writen to fd, keep buffer
    }

    return total_write;
}


void handle_sigpipe() {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));

    sa.sa_handler = SIG_IGN;
    sa.sa_flags = 0;

    sigaction(SIGPIPE, &sa, nullptr);
}


int set_socket_nonblock(int fd) {
    int flag = fcntl(fd, F_GETFL, 0);
    if (flag < 0) { return flag; }
    flag |= O_NONBLOCK;
    return (fcntl(fd, F_SETFL, flag));
}


// Disable the Nagle's algorithm.
void set_socket_nodelay(int fd) {
    int enable = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (void*)&enable, sizeof(enable));
}


// Linger time: the amount of time that the socket will remain open after calling close() .
// For sending the unsent data.
void set_socket_nolinger(int fd) {
    struct linger ling;
    ling.l_onoff = 1;  // enable
    ling.l_linger = 30;  // 30 seconds
    setsockopt(fd, SOL_SOCKET, SO_LINGER, (const char *)&ling, sizeof(ling));
}


// close sending .
void shutdown_WR(int fd) {
    shutdown(fd, SHUT_WR);  // stop transmissions.
}


int socket_bind_listen(int port) {
    if (port < 0 || port > 65535) { return -1; }

    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) { return -1; }

    int en_reuse = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &en_reuse, sizeof(en_reuse)) < 0) {
        close(listenfd);
        return -1;
    }

    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);

    if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        close(listenfd);
        return -1;
    }

    if (listen(listenfd, SOMAXCONN) < 0) {
        close(listenfd);
        return -1;
    }

    if (listenfd < 0) { 
        close(listenfd);
        return -1; 
    }

    return listenfd;
}
