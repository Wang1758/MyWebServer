/*
 * @Author       : mark
 * @Date         : 2020-06-26
 * @copyleft Apache 2.0
 */ 

#ifndef BUFFER_H
#define BUFFER_H
#include <cstring>   //perror
#include <iostream>
#include <unistd.h>  // write
#include <sys/uio.h> //readv
#include <vector> //readv
#include <atomic>
#include <assert.h>
class Buffer {
private:
    // 返回缓冲区起始地址
    char* BeginPtr_();  
    // 返回缓冲区起始地址（常量）
    const char* BeginPtr_() const;  
    // 扩容，如果写区+预备区的大小小于len，则resize，否则将可读区向前移动到缓冲区起始位置
    void MakeSpace_(size_t len);

    std::vector<char> buffer_;
    std::atomic<std::size_t> readPos_;
    std::atomic<std::size_t> writePos_;

public:
    Buffer(int initBuffSize = 1024);
    ~Buffer() = default;
    // 返回可写区的大小       
    size_t WritableBytes() const; 
    // 返回可读区的大小  
    size_t ReadableBytes() const ;  
    // 返回预备区的大小
    size_t PrependableBytes() const;  
    // 返回可读区的起始地址
    const char* Peek() const;
    // 确保写缓冲区长度大于len，否则调用MakeSpace_()扩容
    void EnsureWriteable(size_t len);
    // 移动writePos_指针共len个位置，表示已经写入len个字节
    void HasWritten(size_t len);

    // 读取长度为len的数据，并移动readPos_
    void Retrieve(size_t len);
    // 读取数据一直读到end，并移动readPos_
    void RetrieveUntil(const char* end);

    // 读取所有数据，并清空缓冲区，读写指针归零
    void RetrieveAll() ;
    // 读取所有数据，并返回字符串，同时清空缓冲区，读写指针归零
    std::string RetrieveAllToStr();

    // 返回可写区的起始地址（常量）
    const char* BeginWriteConst() const;
    // 返回可写区的起始地址
    char* BeginWrite();

    // 将str中的数据添加到写缓冲区
    void Append(const std::string& str);
    void Append(const void* data, size_t len);
    // 将buff中的可读区数据添加到写缓冲区
    void Append(const Buffer& buff);
    // 被其他三个Append调用，将str中的数据添加到写缓冲区
    void Append(const char* str, size_t len);

    ssize_t ReadFd(int fd, int* Errno);
    ssize_t WriteFd(int fd, int* Errno);
};

#endif //BUFFER_H