#include "httpconn.h"
using namespace std;

bool HttpConn::isET;
const char* HttpConn::srcDir;
std::atomic<int> HttpConn::userCount;

HttpConn::HttpConn() {
    fd_ = -1;
    addr_ ={0};
    isClose_ = true;
}

HttpConn::~HttpConn() {
    Close();
}

void HttpConn::init(int fd, const sockaddr_in& addr) {
    assert(fd>0);
    userCount++;
    writeBuff_.RetrieveAll();
    readBuff_.RetrieveAll();
    fd_ = fd;
    addr_ = addr;
    isClose_ = false;
    LOG_INFO("Client[%d](%s:%d) in, userCount:%d", fd_, GetIP(), GetPort(), (int)userCount);
}

void HttpConn::Close() {
    response_.UnmapFile();
    if(isClose_ == false) {
        isClose_ = true;
        userCount--;
        close(fd_);
        LOG_INFO("Client[%d](%s:%d) quit, userCount:%d", fd_, GetIP(), GetPort(), (int)userCount);
    }
}

int HttpConn::GetFd() const{
    return fd_;
}

struct sockaddr_in HttpConn::GetAddr() const{
    return addr_;
}

const char* HttpConn::GetIP() const {
    return inet_ntoa(addr_.sin_addr);
}

int HttpConn::GetPort() const {
    return addr_.sin_port;
}

ssize_t HttpConn::read(int* saveErrno) {
    ssize_t len = -1;
    do{
        len = readBuff_.ReadFd(fd_, saveErrno);
        if(len<=0) {
            break;
        }
    } while(isET);  // 如果是边沿触发模式，继续读取直到数据读完
    return len;
}

bool HttpConn::process(){
    request_.Init();
    if (readBuff_.ReadableBytes() <= 0)
    {
        return false;
    }
    else if (request_.parse(readBuff_)) // 处理请求，并初始化响应对象
    {
        LOG_DEBUG("%s", request_.path().c_str());
        response_.Init(srcDir, request_.path(), request_.IsKeepAlive(), 200);
    }
    else
    {
        response_.Init(srcDir, request_.path(), false, 400);
    }

    response_.MakeResponse(writeBuff_); // 将响应的信息写入写缓冲区中
    /* 响应头 */
    iov_[0].iov_base = const_cast<char *>(writeBuff_.Peek());
    iov_[0].iov_len = writeBuff_.ReadableBytes();
    iovCnt_ = 1;

    /* 文件 */
    if (response_.FileLen() > 0 && response_.File())
    {
        iov_[1].iov_base = response_.File();
        iov_[1].iov_len = response_.FileLen();
        iovCnt_ = 2;
    }
    LOG_DEBUG("filesize:%d, %d  to %d", response_.FileLen(), iovCnt_, ToWriteBytes());
    return true;
}

ssize_t HttpConn::write(int *saveErrno){
    ssize_t len = -1;
    do{
        len = writev(fd_, iov_, iovCnt_);
        if (len <= 0){
            *saveErrno = errno;
            break;
        }
        if (iov_[0].iov_len + iov_[1].iov_len == 0){    // 传输结束
            break;
        }else if (static_cast<size_t>(len) > iov_[0].iov_len){  // 第二个缓冲区还有数据没传输
            iov_[1].iov_base = (uint8_t *)iov_[1].iov_base + (len - iov_[0].iov_len);
            iov_[1].iov_len -= (len - iov_[0].iov_len);
            if (iov_[0].iov_len)
            {
                writeBuff_.RetrieveAll();
                iov_[0].iov_len = 0;
            }
        }else{  // 两个缓冲区都还有数据没传输
            iov_[0].iov_base = (uint8_t *)iov_[0].iov_base + len;
            iov_[0].iov_len -= len;
            writeBuff_.Retrieve(len);
        }
    } while (isET || ToWriteBytes() > 10240);   // 如果是ET模式或者剩余传输的文件太大，继续传输
    return len;
}