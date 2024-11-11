#ifndef HTTP_CONN_H
#define HTTP_CONN_H

#include <sys/types.h>
#include <sys/uio.h>   // readv/writev
#include <arpa/inet.h> // sockaddr_in
#include <stdlib.h>    // atoi()
#include <errno.h>

#include "../log/log.h"
#include "../pool/sqlconnRAII.h"
#include "../buffer/buffer.h"
#include "httprequest.h"
#include "httpresponse.h"

class HttpConn
{
public:
    HttpConn();

    ~HttpConn();
    // 初始化HTTP连接，userCount+1
    void init(int sockFd, const sockaddr_in &addr);
    
    //从fd_中读取数据到readBuff_中，返回读取到的总字节数
    ssize_t read(int *saveErrno);
    // 通过writev将读缓冲区和通过mmap映射的内存中的数据发送到fd_中，返回写入到的总字节数
    ssize_t write(int *saveErrno);
    // 析构函数调用，userCount-1
    void Close();

    int GetFd() const;
    int GetPort() const;
    // 返回当前HTTP连接的IPv4点分十进制，返回的指针指向同一个静态区域，多线程可能会导致数据竞争（看看后面的代码再改）
    const char *GetIP() const;
    sockaddr_in GetAddr() const;

    // 根据读取到readBuff_中的数据，通过request_进行解析，在通过response_构造响应报文并填充到writeBuff_中，最后构造iov_
    bool process();

    int ToWriteBytes()
    {
        return iov_[0].iov_len + iov_[1].iov_len;
    }

    bool IsKeepAlive() const
    {
        return request_.IsKeepAlive();
    }

    static bool isET;
    static const char *srcDir;
    static std::atomic<int> userCount;

private:
    int fd_;
    struct sockaddr_in addr_;

    bool isClose_;

    int iovCnt_;
    struct iovec iov_[2];

    Buffer readBuff_;  // 读缓冲区
    Buffer writeBuff_; // 写缓冲区

    HttpRequest request_;
    HttpResponse response_;
};

#endif // HTTP_CONN_H