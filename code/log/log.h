#ifndef LOG_H
#define LOG_H

#include <mutex>
#include <string>
#include <thread>
#include <sys/time.h>
#include <string.h>
#include <stdarg.h>           // vastart va_end
#include <assert.h>
#include <sys/stat.h>         //mkdir
#include "blockqueue.h"
#include "../buffer/buffer.h"

class Log {
public:
    void init(int level, const char* path = "./log", 
                const char* suffix =".log",
                int maxQueueCapacity = 1024);

    static Log* Instance();
    static void FlushLogThread();

    void write(int level, const char *format,...);
    void flush();

    int GetLevel();
    void SetLevel(int level);
    bool IsOpen() { return isOpen_; }
    
private:
    Log();
    void AppendLogLevelTitle_(int level);
    virtual ~Log();
    void AsyncWrite_();

private:
    // log文件路径的最大长度
    static const int LOG_PATH_LEN = 256;
    // log文件名的最大长度
    static const int LOG_NAME_LEN = 256;
    // log文件内的最大行数
    static const int MAX_LINES = 50000;

    // log文件路径
    const char* path_;
    // log后缀名
    const char* suffix_;

    // 最大日志行数
    int MAX_LINES_;

    // 当前行数
    int lineCount_;
    // 日志日期, 用于按天切分日志
    int toDay_;

    bool isOpen_;
 
    // 输出的内容，写入到缓冲区，然后写入到文件
    Buffer buff_;
    // 日志等级
    int level_;
    // 是否异步
    bool isAsync_;

    FILE* fp_;
    // 阻塞队列，用于异步写日志
    std::unique_ptr<BlockDeque<std::string>> deque_; 
    // 写线程的指针
    std::unique_ptr<std::thread> writeThread_;
    // 互斥锁
    std::mutex mtx_;
};

#define LOG_BASE(level, format, ...) \
    do {\
        Log* log = Log::Instance();\
        if (log->IsOpen() && log->GetLevel() <= level) {\
            log->write(level, format, ##__VA_ARGS__); \
            log->flush();\
        }\
    } while(0);

// 四个宏定义，主要用于不同类型的日志输出，也是外部使用日志的接口
// ...表示可变参数，__VA_ARGS__就是将...的值复制到这里
// 前面加上##的作用是：当可变参数的个数为0时，这里的##可以把把前面多余的","去掉,否则会编译出错
#define LOG_DEBUG(format, ...) do {LOG_BASE(0, format, ##__VA_ARGS__)} while(0);
#define LOG_INFO(format, ...) do {LOG_BASE(1, format, ##__VA_ARGS__)} while(0);
#define LOG_WARN(format, ...) do {LOG_BASE(2, format, ##__VA_ARGS__)} while(0);
#define LOG_ERROR(format, ...) do {LOG_BASE(3, format, ##__VA_ARGS__)} while(0);

#endif //LOG_H