#ifndef EPOLLER_H
#define EPOLLER_H

#include <sys/epoll.h> //epoll_ctl()
#include <fcntl.h>     // fcntl()
#include <unistd.h>    // close()
#include <assert.h>
#include <vector>
#include <errno.h>
#include <stdexcept>

class Epoller
{
public:
    explicit Epoller(int maxEvent = 1024);

    ~Epoller();

    bool AddFd(int fd, uint32_t events);

    bool ModFd(int fd, uint32_t events);
    // 删除fd上的所有事件
    bool DelFd(int fd);
    
    int Wait(int timeoutMs = -1);

    int GetEventFd(size_t i) const;

    uint32_t GetEvents(size_t i) const;

private:
    int epollFd_;

    std::vector<struct epoll_event> events_;
};

#endif