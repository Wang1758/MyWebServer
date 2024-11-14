# 服务端设计
### 1. epoller
##### 1.1 epoll的接口
```
#include <sys/epoll.h>
// 创建一个新的epoll实例。在内核中创建了一个数据，这个数据中有两个比较重要的数据，一个是需要检测的文件描述符的信息（红黑树），还有一个是就绪列表，存放检测到数据发送改变的文件描述符信息（双向链表）。
int epoll_create(int size);
	- 参数：
		size : 目前没有意义了。随便写一个数，必须大于0
	- 返回值：
		-1 : 失败
		> 0 : 文件描述符，操作epoll实例的
            
typedef union epoll_data {
	void *ptr;
	int fd;
	uint32_t u32;
	uint64_t u64;
} epoll_data_t;

struct epoll_event {
	uint32_t events; /* Epoll events */
	epoll_data_t data; /* User data variable */
};
    
// 对epoll实例进行管理：添加文件描述符信息，删除信息，修改信息
int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
	- 参数：
		- epfd : epoll实例对应的文件描述符
		- op : 要进行什么操作
			EPOLL_CTL_ADD: 添加
			EPOLL_CTL_MOD: 修改
			EPOLL_CTL_DEL: 删除
		- fd : 要检测的文件描述符
		- event : 检测文件描述符什么事情
            
// 检测函数
int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout);
	- 参数：
		- epfd : epoll实例对应的文件描述符
		- events : 传出参数，保存了发生了变化的文件描述符的信息
		- maxevents : 第二个参数结构体数组的大小
		- timeout : 阻塞时间
			- 0 : 不阻塞
			- -1 : 阻塞，直到检测到fd数据发生变化，解除阻塞
			- > 0 : 阻塞的时长（毫秒）
	- 返回值：
		- 成功，返回发送变化的文件描述符的个数 > 0
		- 失败 -1
```

##### 1.2 常见的epoll检测事件
(1) **EAGAIN**: 如果socket的状态为非阻塞，但是accept函数没有找到可用的连接，就会返回EAGAIN错误
应用一： accept
常在死循环中设置if(errno==EAGAIN) break;
因为死循环只是为了接收缓冲区的连接，一旦accept在缓冲区找不到可用的连接，那么accept就会将errno设置为EAGAIN

应用二： recv、send
前提：非阻塞的IO、EPOLLET边缘触发模式
recv：在EPOLLIN|EPOLLET监视可读、边缘触发模式下，recv函数会时刻关注缓冲区中是否有数据可读，如果缓冲区中有数据未处理，EPOLLET模式下的epoll只会汇报一次socket有可读事件，当有新的数据加入缓冲区时，就会再次汇报可读。
send：在EPOLLOUT|EPOLLET监视可写、边缘触发模式下，send函数会时刻关注发送缓冲区是否已满，如果发送缓冲区未满，EPOLLET模式下就会触发一次可写事件，只有当缓冲区从满变为"有空"的时候，才会再次触发一次可写事件
根据上面的说明，假设缓冲区1000字节就满了，现如今缓冲区中有300字节的内容，那么ET模式会触发一次可写事件，在这个可写时间的处理代码中，你可以向客户端发送你想要发送的内容。当写入缓冲区满了的时候，因为是非阻塞IO，send会立即返回，并将errno设置为EAGAIN，此时的EAGAIN表示缓冲区内容已满，无法继续写入，所以我们也没必要让send阻塞在那里。

(2) **其他事件**
POLLIN ：表示对应的文件描述符可以读（包括对端 SOCKET 正常关闭）；
EPOLLOUT：表示对应的文件描述符可以写；触发这个事件的场景：
- 客户端connect上服务端后，这时候把fd添加到epoll 事件池里面后，因为连接可写，会触发EPOLLOUT事件
- 内核缓冲区从满到不满，会触发EPOLLOUT事件,典型应用是数据包发包问题，当send只部分成功时，通过epoll检测EPOLLOUT事件再继续通过send发送
- 重新注册EPOLLOUT事件，当连接可用，且缓冲区不满，调用epoll_ctl将fd重新注册到epoll事件池中也会触发EPOLLOUT事件
EPOLLPRI：表示对应的文件描述符有紧急的数据可读（这里应该表示有带外数据到来）；
EPOLLERR：表示对应的文件描述符发生错误；
EPOLLHUP：表示对应的文件描述符被挂断；
EPOLLET：将EPOLL设为边缘触发(Edge Triggered)模式，这是相对于水平触发(Level Triggered)来说的。
EPOLLONESHOT：只监听一次事件，当监听完这次事件之后，如果还需要继续监听这个socket的话，需要再次把这个socket加入到EPOLL队列里。
1. 客户端直接调用close，会触发EPOLLRDHUP事件
2. 通过EPOLLRDHUP属性，来判断是否对端已经关闭，这样可以减少一次系统调用。


### 2. WebServer
##### 1. 初始化web服务器
构造函数： 
1. 设置服务器参数、初始化定时器/线程池/反应堆/连接队列
2. 如果参数openlog = true， 则初始化一个日志系统对象实例
3. 调用InitEventMode_初始化监听时间与连接事件，传入参数trigMode可选值为0-3
4. 调用InitSocket_初始化服务端的监听socketfd，添加监听事件EPOLLIN

##### 2. Start
通过start函数启动服务器的主循环，每次循环开始定时器都将搏动一次，删除超时连接。epoller_调用wait（）来监听事件的到来，直到超时或者有事件到来
不同事件的处理逻辑：
1. 收到新的http请求
调用函数DealListen_()，接受客户端的连接
2. 已经建立的http上的IO请求
在events& EPOLLIN 或events & EPOLLOUT为真时，需要进行读写的处理。分别调用 DealRead_(&users_[fd])和DealWrite_(&users_[fd]) 函数将该可读、可写事件放入线程池的等待队列中
同时调用 ExtentTime_函数重置该客户端的超时时间
3. 线程处理IO事件
读、写事件将分别调用OnRead_、OnWrite_函数进行分散读与集中写。

从步骤1、2中可以看出这个服务器是Reactor模式，连接事件由服务器处理，IO事件交给工作线程处理

##### 3. IO处理流程
1. 客户端连接后服务器会将其可读事件放入反应堆中
2. 当客户端通过send发送数据时，服务端监听到可读事件，并通过一个子线程处理。
3. 子线程将调用OnRead_函数将客户端发送的数据写入到该客户端连接对应的读缓冲区，调用process函数进行处理，构造应答报文并放在写缓冲区
4. 在之后将客户端的EPOLLOUT事件添加到反应堆，此时会立即触发EPOLLOUT事件
5. 触发EPOLLOUT事件后，服务端通过一个子线程处理这个可写事件
6. 子线程调用OnWrite_函数将构造的应答报文发送给客户端，如果此时内核缓冲区满了，会再次将该客户端的EPOLLOUT事件添加到反应堆，再发送一次
7. 发送完数据后，如果该客户端是一个持续连接，则将该客户端的EPOLLIN事件添加到反应堆

##### 4. 客户端关闭
以下情况会关闭客户端的连接
- 服务器过载，即客户数量大于MAX_FD（65536），此时还未给该客户分配资源，直接close这个socketfd就行
- 客户端超时，即客户端长时间没有触发可读或可写事件，此时在时间堆中该客户端被判定超时，将调用CloseConn_释放这个客户端的资源
- OnRead_函数执行时发生异常，即客户端调用readv时返回值是负数，且并没有触发EAGAIN事件（读缓冲区是空的时触发）
- OnWrite_执行后，如果没有重新监听EPOLLOUT事件或者客户端不是一个持续连接，那么该函数将正常执行到最后一步，即释放这个客户端的资源。这种情况下是客户端为非持续连接