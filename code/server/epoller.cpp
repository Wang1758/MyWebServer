#include "epoller.h"

Epoller::Epoller(int maxEvent) : epollFd_(epoll_create(100)), events_(maxEvent) {
    if(epollFd_ < 0 || events_.size() <=0) {
        throw std::runtime_error("Failed to initialize Epoller: invalid file descriptor or event size.");
    }
}

Epoller::~Epoller() {
    close(epollFd_);
}

bool Epoller::AddFd(int fd, uint32_t events) {
    if(fd<0) {
        return false;
    }
    epoll_event ev = {0};
    ev.data.fd = fd;
    ev.events = events;
    return 0 == epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &ev);
}

bool Epoller::ModFd(int fd, uint32_t events) {
    if(fd<0){
        return false;
    }
    epoll_event ev = {0};
    ev.data.fd = fd;
    ev.events = events;
    return 0 == epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd, &ev);
}

bool Epoller::DelFd(int fd) {
    if(fd<0) {
        return false;
    }
    epoll_event ev = {0};
    return 0 == epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, &ev);
}

int Epoller::Wait(int timeoutMs) {
    return epoll_wait(epollFd_, &events_[0], static_cast<int>(events_.size()), timeoutMs);
}

int Epoller::GetEventFd(size_t i) const {
    if(i>=events_.size() || i<0){
        throw std::out_of_range("i不在events范围内");
    }
    return events_[i].data.fd;
}

uint32_t Epoller::GetEvents(size_t i) const {
    if (i >= events_.size() || i < 0){
        throw std::out_of_range("i不在events范围内");
    }
    return events_[i].events;
}
