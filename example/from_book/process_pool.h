#ifndef PROCESSPOOL_H
#define PROCESSPOOL_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cassert>
#include <cstdio>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <cstdlib>
#include <sys/epoll.h>
#include <csignal>
#include <sys/wait.h>
#include <sys/stat.h>

class process {
  public:
    process() : m_pid(-1) {}

  public:
    pid_t m_pid;     // 目标子进程的pid
    int m_pipefd[2]; // 父子进程通信管道
};


template<typename T>
class processpool {
  private:
    explicit processpool(int listenfd, int process_number = 8);

  public:
    // 单例模式，只存在一个实例，这是正确处理信号的必要条件
    static processpool<T> *create(int listenfd, int process_number = 8) {
        if (!m_instance) {
            m_instance = new processpool<T>(listenfd, process_number);
        }
        return m_instance;
    }
    ~processpool() { delete[] m_sub_process; }
    void run();

  private:
    void setup_sig_pipe();
    void run_parent();
    void run_child();

  private:
    static const int MAX_PROCESS_NUMBER = 16;
    static const int USER_PER_PROCESS = 65536;
    static const int MAX_EVENT_NUMBER = 10000;
    int m_process_number;  
    int m_idx;   // 子进程 index
    int m_epollfd;  // 每个进程一个 epoll
    int m_listenfd;
    int m_stop;   // 子进程以此决定是否停止
    process *m_sub_process;  // 保存所有子进程的描述信息
    static processpool<T> *m_instance;   // 进程池静态实例
};

template<typename T>
processpool<T> *processpool<T>::m_instance = NULL;

// 处理信号的管道，用于统一事件源
static int sig_pipefd[2];

static int setnonblocking(int fd) {
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

static void addfd(int epollfd, int fd) {
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

static void removefd(int epollfd, int fd) {
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
}

static void sig_handler(int sig) {
    int save_errno = errno;
    int msg = sig;
    send(sig_pipefd[1], (char *)&msg, 1, 0);
    errno = save_errno;
}

static void addsig(int sig, void(handler)(int), bool restart = true) {
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;
    if (restart) { sa.sa_flags |= SA_RESTART; }
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL) != -1);
}


template<typename T>
processpool<T>::processpool(int listenfd, int process_number)
    : m_listenfd(listenfd)
    , m_process_number(process_number)
    , m_idx(-1)
    , m_stop(false) {
    assert((process_number > 0) && (process_number <= MAX_PROCESS_NUMBER));

    m_sub_process = new process[process_number];
    assert(m_sub_process);

    // 创建子进程，并建立与父进程通信的管道
    for (int i = 0; i < process_number; ++i) {
        int ret = socketpair(PF_UNIX, SOCK_STREAM, 0, m_sub_process[i].m_pipefd);
        assert(ret == 0);

        m_sub_process[i].m_pid = fork();
        assert(m_sub_process[i].m_pid >= 0);
        if (m_sub_process[i].m_pid > 0) {
            close(m_sub_process[i].m_pipefd[1]);
            continue;
        } else {
            close(m_sub_process[i].m_pipefd[0]);  // 全双工管道，无所谓读写段，都可以读写
            m_idx = i;
            break;
        }
    }
}

// 统一事件源
template<typename T>
void processpool<T>::setup_sig_pipe() {
    m_epollfd = epoll_create(5);
    assert(m_epollfd != -1);

    int ret = socketpair(PF_UNIX, SOCK_STREAM, 0, sig_pipefd);
    assert(ret != -1);

    setnonblocking(sig_pipefd[1]);
    addfd(m_epollfd, sig_pipefd[0]);

    addsig(SIGCHLD, sig_handler);
    addsig(SIGTERM, sig_handler);
    addsig(SIGINT, sig_handler);
    addsig(SIGPIPE, SIG_IGN);
}

// 父进程中 m_idx 为 -1
template<typename T>
void processpool<T>::run() {
    if (m_idx != -1) {
        run_child();
        return;
    }
    run_parent();
}

template<typename T>
void processpool<T>::run_child() {
    setup_sig_pipe();   // 在多进程、多线程环境中，要以进程、线程为单位来处理信号和信号掩码。
                        // 虽然子进程会集成掩码，但是有独立的挂起信号集
                        // 原进程设置的信号处理函数不再对新进程起作用

    // 子进程通过在进程池中的index找到与父进程通信的管道
    int pipefd = m_sub_process[m_idx].m_pipefd[1];
    // 子进程监听 pipefd ，父进程会通知子进程 accept 新连接
    addfd(m_epollfd, pipefd);

    epoll_event events[MAX_EVENT_NUMBER];
    T *users = new T[USER_PER_PROCESS];
    assert(users);
    int number = 0;
    int ret = -1;

    while (!m_stop) {
        number = epoll_wait(m_epollfd, events, MAX_EVENT_NUMBER, -1);
        if ((number < 0) && (errno != EINTR)) {
            printf("epoll failure\n");
            break;
        }

        for (int i = 0; i < number; i++) {
            int sockfd = events[i].data.fd;
            if ((sockfd == pipefd) && (events[i].events & EPOLLIN)) {
                int client = 0;
                // 读取数据，表示有新客户端连接到来
                ret = recv(sockfd, (char *)&client, sizeof(client), 0);
                if (((ret < 0) && (errno != EAGAIN)) || ret == 0) {
                    continue;
                } else {
                    struct sockaddr_in client_address;
                    socklen_t client_addrlength = sizeof(client_address);
                    int connfd = accept(
                        m_listenfd, (struct sockaddr *)&client_address,
                        &client_addrlength);
                    if (connfd < 0) {
                        printf("errno is: %d\n", errno);
                        continue;
                    }
                    addfd(m_epollfd, connfd);
                    // 模板类需要实现 init 方法，connfd 来索引客户端连接
                    users[connfd].init(m_epollfd, connfd, client_address);
                }
            } else if ((sockfd == sig_pipefd[0]) && (events[i].events & EPOLLIN)) {
                int sig;
                char signals[1024];
                // 父子进程的 sig_pipefd 是两个不同的，在两个空间
                ret = recv(sig_pipefd[0], signals, sizeof(signals), 0);
                if (ret <= 0) {
                    continue;
                } else {
                    for (int i = 0; i < ret; ++i) {
                        switch (signals[i]) {
                            case SIGCHLD: {
                                pid_t pid;
                                int stat;
                                while ((pid = waitpid(-1, &stat, WNOHANG)) > 0) {
                                    continue;
                                }
                                break;
                            }
                            case SIGTERM:
                            case SIGINT: {
                                m_stop = true;
                                break;
                            }
                            default: {
                                break;
                            }
                        }
                    }
                }
            } 
            // 客户请求到达，处理请求
            else if (events[i].events & EPOLLIN) {
                users[sockfd].process();
            } else {
                continue;
            }
        }
    }

    delete[] users;
    users = NULL;
    close(pipefd);
    // close( m_listenfd );  // 哪个函数创建，哪个函数销毁
    close(m_epollfd);
}


template<typename T>
void processpool<T>::run_parent() {
    setup_sig_pipe();

    // 父进程监听 listenfd
    addfd(m_epollfd, m_listenfd);

    epoll_event events[MAX_EVENT_NUMBER];
    int sub_process_counter = 0;
    int new_conn = 1;
    int number = 0;
    int ret = -1;

    while (!m_stop) {
        number = epoll_wait(m_epollfd, events, MAX_EVENT_NUMBER, -1);
        if ((number < 0) && (errno != EINTR)) {
            printf("epoll failure\n");
            break;
        }

        for (int i = 0; i < number; i++) {
            int sockfd = events[i].data.fd;
            if (sockfd == m_listenfd) {
                // round robin 分配给一个子进程处理
                int i = sub_process_counter;
                do {
                    if (m_sub_process[i].m_pid != -1) { break; }
                    i = (i + 1) % m_process_number;
                } while (i != sub_process_counter);

                if (m_sub_process[i].m_pid == -1) {
                    m_stop = true;
                    break;
                }
                sub_process_counter = (i + 1) % m_process_number;
                send(m_sub_process[i].m_pipefd[0], (char *)&new_conn,sizeof(new_conn), 0);
                printf("send request to child %d\n", i);
            } 
            else if ((sockfd == sig_pipefd[0]) && (events[i].events & EPOLLIN)) {
                int sig;
                char signals[1024];
                ret = recv(sig_pipefd[0], signals, sizeof(signals), 0);
                if (ret <= 0) {
                    continue;
                } else {
                    for (int i = 0; i < ret; ++i) {
                        switch (signals[i]) {
                            case SIGCHLD: {
                                pid_t pid;
                                int stat;
                                while ((pid = waitpid(-1, &stat, WNOHANG)) > 0) {
                                    for (int i = 0; i < m_process_number; ++i) {
                                        if (m_sub_process[i].m_pid == pid) {
                                            printf("child %d join\n", i);
                                            close(m_sub_process[i].m_pipefd[0]);
                                            m_sub_process[i].m_pid = -1;
                                        }
                                    }
                                }
                                // 所有子进程退出前提下，父进程也退出
                                m_stop = true;
                                for (int i = 0; i < m_process_number; ++i) {
                                    if (m_sub_process[i].m_pid != -1) {
                                        m_stop = false;
                                    }
                                }
                                break;
                            }
                            case SIGTERM:
                            case SIGINT: {
                                // 父进程收到终止信号，杀死所用子进程，更好的方式是向子进程管道发送特殊数据
                                printf("kill all the clild now\n");
                                for (int i = 0; i < m_process_number; ++i) {
                                    int pid = m_sub_process[i].m_pid;
                                    if (pid != -1) { kill(pid, SIGTERM); }
                                }
                                break;
                            }
                            default: {
                                break;
                            }
                        }
                    }
                }
            } else {
                continue;
            }
        }
    }

    // close( m_listenfd );
    close(m_epollfd);
}

#endif
