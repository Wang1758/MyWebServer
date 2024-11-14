# WebServer
基于c++实现的高性能Web服务器

# 已实现功能
* 使用epoll+非阻塞IO以及线程池实现了高并发的Reactor模型
* 提供了一个可动态管理的缓冲区，通过分散读与集中写来高效管理网络数据的读写
* 利用单例模式与阻塞队列实现的异步日志系统，记录服务器的运行状态
* 使用RAII机制实现了数据库连接池，减少了数据库连接建立与关闭的开销，同时实现了用户登录注册功能
* 利用正则表达式与有限状态机解析HTTP请求报文，目前支持GET和POST方法，可处理静态资源
* 使用小根堆实现了一个定时器，定期回调函数关闭超时连接
* 使用C++11新特性如通过智能指针来管理动态内存，lambda表达式简化代码，右值引用和std：：move操作优化对象移动操作等

# 项目启动
### 1. 配置数据库
```
// 建立一个数据库，库名自定义如mydb
create database mydb;

// 创建user表
USE mydb;
CREATE TABLE user(
    username char(50) NULL,
    password char(50) NULL
)ENGINE=InnoDB;

// 添加账户（非必须，自己注册一个是一样的操作）
INSERT INTO user(username, password) VALUES('name', 'password');
```

### 2. 配置自己服务器数据
打开/code/main.cpp，参数解释如下（仅中文处需要修改）：
```
WebServer server(服务器端口号, 3, 60000, false,
                 mysql占用端口号, mysql用户名, mysql密码, 数据库名,
                 15, 6, true, 1, 1000);
```

### 2. 搭建项目
在‘MyWeberver/’目录输入make来搭建项目
搭建完成后会在根目录生成一个bin文件夹，bin/server为可执行程序

### 3. 执行程序
```
./bin/server
```

### 4. 客户端启动
在linux或windows中打开一个浏览器，地址栏中输入：
```
[服务器的IPV4地址] : [服务器端口号]
```
