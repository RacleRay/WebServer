#pragma once

#include <cstdlib>
#include <string>

ssize_t readn(int fd, void *buff, size_t n);
ssize_t read_utill_nodata(int fd, std::string &buff, bool &nodata);
ssize_t read_utill_nodata_nflag(int fd, std::string &buff);

ssize_t writen(int fd, void *buff, size_t n);
ssize_t writen(int fd, std::string &buff);

void handle_sigpipe();

int set_socket_nonblock(int fd);
void set_socket_nodelay(int fd);
void set_socket_nolinger(int fd);

void shutdown_WR(int fd);

int socket_bind_listen(int port);