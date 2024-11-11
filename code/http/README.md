# HTTP处理步骤

### 1. HttpConn.init(int sockFd, const sockaddr_in &addr)
传入客户端的sockfd，以及客户端的ipv4地址结构体，初始化http连接
静态变量userCount+1

### 2. HttpConn.read()
调用readBuff_.ReadFd，从sockfd中读取客户端发送的http请求报文，该函数将一次性读完并放到readbuff_中

### 3. HttpConn.process()
首先初始化request_，通过request_.parse(readBuff_)处理读缓冲区的http请求报文，然后将处理的结果放在request_的成员变量中
接着根据request_解析的请求，初始化response_，通过response_.MakeResponse(writeBuff_)构造响应报文，将响应头放在writebuff_中，响应体映射到内存中
初始化iov_为writebuff_和响应体指针

### 4. HttpConn.write()
通过writev将iov_中的数据发送到sockfd，如果文件太大或者是ET模式，那么会继续发送还未发送的数据

### 5. HttpConn.Close()
HttpConn对象析构时调用，将释放response_中通过mmap映射到内存的数据，并设置HttpConn成员变量isClose_为true
静态变量userCount-1
关闭sockfd