# Simple web Server

> 更清晰简短的服务端实例 [《linux高性能服务器编程》.游双](./example/from_book/) 

- One loop per thread，每个线程一个处理连接的业务逻辑，多线程+非阻塞IO多路复用，这么做也有效地处理了惊群效应的发生
- eventfd 异步唤醒对等线程处理线程独占的任务队列中的回调函数
- 相比这位朋友的 [WebServer](https://github.com/linyacool/WebServer) 优化了网络连接过程处理逻辑，短连接可以稳定且更高效地断开连接
- 使用双缓冲区设计，减少日志写入磁盘的频率
- 多线程负载均衡方式，使用简单的 Round Robin 循环取模以此分发任务
- 边缘触发+非阻塞IO，这是提高并发能力所必须的
- 简单的定时器堆管理，优先关闭剩余时限最小的连接
- Epoll 事件注册与处理逻辑，没有做到简洁明了，因此对于程序的调试和理解，可能不太友好，这点有待优化
- 使用C++语言的一些特性，优化了部分程序编写逻辑
- 增加了简单的读取配置文件的模块

总体而言，在我的虚拟机（配置2核4G）内，4线程服务器程序，QPS内达到了30000+，还是不错的。

> 程序整体基本完工，但是优化后调试着实是有点麻烦的。网络程序本来调试起来不那么直观。善用打印日志，熟悉基本原理，真的是会事半功倍。另外，别太相信别人说的，自己做出来了才是真的。

> 测试使用 [WebBench](https://github.com/EZLippi/WebBench) 项目

### 系统设置及调试

最大文件描述符限制，用户级限制：限制所属用户的所有进程打开的文件描述符数量；系统级限制：限制所用用户打开的文件描述符数量。

```sh
ulimit -n

ulimit -SHn max-file-number

# /etc/security/limits/conf 中加入
* hard nofile max-file-number
* soft nofile max-file-number

# 系统级限制修改
sysctl -w fs.file-max=max-file-number
```

#### /proc/sys

```sh
# 查看内核参数
sysctl -a
```

#### /proc/sys/fs

```sh
/proc/sys/fs/file-max
# inode-max 设置为 file-max 的 3-4 倍
/proc/sys/fs/inode-max
# 一个用户打开的所有 epoll 实例，总共能监听的事件数量，而不是单个 epoll 的事件数目
/proc/sys/fs/epoll/max-user-watches
```

epoll 中注册一个事件，32位系统消耗 90 字节内核空间，64位系统消耗160字节内核空间。



#### /proc/sys/net

```sh
# listen 监听队列，还未建立 ESTABLISHED 状态 socket 的最大数目
/proc/sys/net/core/somaxconn

# listen 监听队列，SYN_RCVD 状态的 socket 最大数目
/proc/sys/net/ipv4/tcp_max_syn_backlog

# TCP 写缓冲区的最小值、默认值、最大值
/proc/sys/net/ipv4/tcp_wmem 

# TCP 读缓冲区的最小值、默认值、最大值
/proc/sys/net/ipv4/tcp_rmem

# 通过 cookie 防止一个监听 socket 不停重复接收来自同一个地址的连接请求
/proc/sys/net/ipv4/tcp_syncookies
```

可以通过 sysctl 命令修改，或者直接改文件，但是都是临时的。

永久的方式是修改，/etc/sysctl.conf 文件，并 sysctl -p 使之生效。



### GDB

#### 多进程

GDB 调试多进程，fork后会停留在原进程，子进程独立运行。

进入子进程：找到目标进程号，gdb 中 attach PID 进入进程。然后 b 打断点，c 运行，bt 查看函数调用栈。

或者，在 gdb 中设置 `set follow-fork-mode parent|child`，设置 fork 后进入哪个进程。



#### 多线程

gdb 中

```sh
# 显示当前可调用所用线程
info threads

# 指定线程
thread [thread id]

# 指定调试时，其他线程的行为
set scheduler-locking [off|on|step]
# off: 不锁定其他线程
# on: 只执行指定的线程
# step: 单步执行时，只用当前线程执行
```

调试进程池或者线程池，一般先将进程或者线程个数设置为1，运行没问题，再增加数量，检查同步行为。


#### 系统监测工具

- tcpdump : -i 监听网卡接口
- nc : netcat -C HTTP 使用 -C 选项会自动添加 <CR><LF> 结尾。
- strace : 跟踪程序运行过程中执行的系统调用和接收到的信号
- lsof  : list open file 列出当前系统打开的文件描述符，-c 显示指定的命令打开的文件描述符
- netstat : 打印本地网卡接口上的全部连接、路由表信息、网卡接口信息等
- vmstat : 实时输出系统资源信息
- ifstat : 网络流量监测工具
- mpstat :  监测多处理器系统中，每个CPU使用情况