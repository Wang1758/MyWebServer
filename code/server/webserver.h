#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <unordered_map>
#include <fcntl.h>  // fcntl()
#include <unistd.h> // close()
#include <assert.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "epoller.h"
#include "../log/log.h"
#include "../timer/heaptimer.h"
#include "../pool/sqlconnpool.h"
#include "../pool/threadpool.h"
#include "../pool/sqlconnRAII.h"
#include "../http/httpconn.h"

class WebServer
{
public:
    WebServer(
        int port, int trigMode, int timeoutMS, bool OptLinger,
        int sqlPort, const char *sqlUser, const char *sqlPwd,
        const char *dbName, int connPoolNum, int threadNum,
        bool openLog, int logLevel, int logQueSize);

    ~WebServer();   
    // 启动服务器主循环
    void Start();

private:
    // 初始化服务器监听的socket，并设置为非阻塞
    bool InitSocket_();
    // 初始化监听事件与连接事件
    void InitEventMode_(int trigMode);
    // 将客户端fd添加到users_中，并分配资源
    void AddClient_(int fd, sockaddr_in addr);

    // 处理客户端连接事件
    void DealListen_();
    // 处理客户端可写事件
    void DealWrite_(HttpConn *client);
    // 处理客户端可读事件
    void DealRead_(HttpConn *client);

    // 向客户端fd发送错误信息info，并关闭fd
    void SendError_(int fd, const char *info);
    // 重置客户端client的过期时间
    void ExtentTime_(HttpConn *client);
    // 关闭客户端client，并释放这个链接的资源
    void CloseConn_(HttpConn *client);

    // 将客户端发送的数据保存在写缓冲区，调用OnProcess处理
    void OnRead_(HttpConn *client);
    // 将客户端写缓冲区的数据发送给客户端，如果客户端是持续连接，则调用OnProcess将监听事件转换为监听写
    // 如果写缓冲区满导致EAGAIN事件，则将该客户端的可读事件再次添加到反应堆中
    void OnWrite_(HttpConn *client);
    // 调用process处理客户端请求报文，并构造响应报文存储在写缓冲区中，同时转换客户端监听的事件
    void OnProcess(HttpConn *client);

    static const int MAX_FD = 65536;    // 当静态成员变量是const类型，不会在类外部引用，并且是整型，则可以在头文件中声明和定义

    // 设置fd为非阻塞模式，返回旧的模式
    static int SetFdNonblock(int fd);

    int port_;
    int timeoutMS_; /* 毫秒MS */
    bool openLinger_;
    bool isClose_;
    int listenFd_;
    char *srcDir_;

    uint32_t listenEvent_;  // 监听事件
    uint32_t connEvent_;    // 连接事件

    std::unique_ptr<ThreadPool> threadpool_;
    std::unique_ptr<HeapTimer> timer_;
    std::unique_ptr<Epoller> epoller_;
    std::unordered_map<int, HttpConn> users_;
};

#endif // WEBSERVER_H