#pragma once

#include <memory>
#include <sys/epoll.h>
#include <unordered_map>
#include <vector>

class Epoll {
public:
    // epoll event management


private:
    static const int MAX_FDS = 100'000;
    
    int m_epoll_fd;

    // events management

    // fd to Channel management

    // fd to HttpData management

    // timer management
};