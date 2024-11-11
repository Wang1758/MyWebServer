#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <unordered_map>
#include <fcntl.h>    // open
#include <unistd.h>   // close
#include <sys/stat.h> // stat
#include <sys/mman.h> // mmap, munmap

#include "../buffer/buffer.h"
#include "../log/log.h"

class HttpResponse
{
public:
    HttpResponse();
    ~HttpResponse();

    void Init(const std::string &srcDir, std::string &path, bool isKeepAlive = false, int code = -1);
    // 检查文件状态并设置状态码code_，调用其他私有函数将响应报文写入buff中
    void MakeResponse(Buffer &buff);
    // 解除mmap建立的额内存映射区域
    void UnmapFile();
    // 返回mmap建立的内存映射区域首地址
    char *File();
    // 返回文件的长度
    size_t FileLen() const;
    void ErrorContent(Buffer &buff, std::string message);
    int Code() const { return code_; }

private:
    // 添加响应状态行
    void AddStateLine_(Buffer &buff);
    // 添加消息报头
    void AddHeader_(Buffer &buff);
    // 添加响应体，通过内存映射提高速度
    void AddContent_(Buffer &buff);

    // 检查当前code是否在CODE_PATH中，如果在，则更改mmFileStat_映射的文件
    void ErrorHtml_();
    // 判断文件类型，若不在SUFFIX_TYPE中定义，则返回默认类型“text/plain”
    std::string GetFileType_();

    int code_;
    bool isKeepAlive_;

    std::string path_;
    std::string srcDir_;

    char *mmFile_;
    struct stat mmFileStat_;

    // 后缀类型集
    static const std::unordered_map<std::string, std::string> SUFFIX_TYPE;
    // 编码状态集
    static const std::unordered_map<int, std::string> CODE_STATUS;
    // 编码路径集  
    static const std::unordered_map<int, std::string> CODE_PATH;
};

#endif // HTTP_RESPONSE_H