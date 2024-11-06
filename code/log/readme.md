## 日志系统
#### 1. 单例模式
   单例模式涉及到一个单一的类，该类负责创建自己的对象，同时确保只有单个对象被创建。这个类提供了一种访问其唯一对象的方式，可以直接访问而不需要实例化该类的对象

1.1 单例模式的实现方式
* **饿汉式**：只有该类被加载并调用getInstance的时候才会初始化这个单例。**在C++11之后，不需要加锁，直接使用函数内的局部静态对象**
* **饱汉模式**：在程序运行时就初始化单例对象。**不需要加锁就可以保证线程安全**

#### 2. 异步日志
* **同步日志**：日志写入函数与工作线程串行执行，由于涉及到I/O操作，当单条日志比较大的时候，同步模式会阻塞整个处理流程，服务器所能处理的并发能力将有所下降，尤其是在峰值的时候，写日志可能成为系统的瓶颈。
* **异步日志**：将所写的日志内容先存入阻塞队列中，写线程从阻塞队列中取出内容，写入日志

日志线程是典型的生产者消费者模型。他的需求是是不是会有一段日志放到阻塞队列中，当日志线程有空的时候会被取出来一段。
因此需要使用加锁、条件变量来实现这个队列

#### 3. 日志系统的运行流程

#### 4. 日志系统的分级与分文件