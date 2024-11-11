#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include<unordered_map>
#include<unordered_set>
#include<string>
#include<regex>
#include<error.h>
#include<mysql/mysql.h>

#include "../buffer/buffer.h"
#include "../log/log.h"
#include "../pool/sqlconnpool.h"
#include "../pool/sqlconnRALL.h"

class HttpRequest {
public:
    // 解析状态，请求行，请求头，请求体，解析完成
    enum PARSE_STATE
    {
        REQUEST_LINE,
        HEADERS,
        BODY,
        FINISH,
    };

    enum HTTP_CODE
    {
        NO_REQUEST = 0,     // 请求不完整，需要继续读取
        GET_REQUEST,        // 获得一个完整的请求
        BAD_REQUEST,        // 请求语法错误
        NO_RESOURSE,        //  没有资源
        FORBIDDENT_REQUEST, // 禁止请求
        FILE_REQUEST,       // 文件请求
        INTERNAL_ERROR,     // 内部错误
        CLOSED_CONNECTION,  // 连接关闭
    };

    HttpRequest() {Init();};
    ~HttpRequest() = default;

    void Init();
    // 解析请求,请求在buff中的读缓冲区
    bool parse(Buffer& buff);

    std::string path() const;
    std::string& path();
    std::string method() const;
    std::string version() const;
    std::string GetPost(const std::string& key) const;
    std::string GetPost(const char* key) const;

    // 是否是持续连接
    bool IsKeepAlive() const;

private:
    // 处理请求行，将结果保存在method_,path_,version_中，解析状态state_转换为HEADERS
    bool ParseRequestLine_(const std::string& line);
    // 处理请求头，如果是最后一行，解析状态state_转换为BODY
    void ParseHeader_(const std::string& line);
    // 处理请求体，将结果保存在body_中，并调用ParsePost_，解析状态转换为FINISH
    void ParseBody_(const std::string& line);

    // 解析和规范化HTTP请求路径，如果请求路径是默认路径，扩展其文件名（加上.html)
    void ParsePath_();
    // 解析post请求中的表单数据，目前可以解析用户登录或注册信息
    void ParsePost_();
    // 从url中解析编码，并存放在post_中，目前仅支持登录或注册
    void ParseFromUrlencode_();

    // 验证用户身份，并区分处理登录和注册行为
    static bool UserVerify(const std::string& name, const std::string& pwd, bool isLogin);

    // 当前请求的解析状态
    PARSE_STATE state_;
    std::string method_,path_,version_,body_;
    std::unordered_map<std::string, std::string> header_;   //请求头的键值对
    std::unordered_map<std::string, std::string> post_;     //请求体的键值对，分别为用户名、密码的键值对

    // 该静态函数返回默认的html路径，检查请求的路径是否在默认的HTML页面集合中
    static const std::unordered_set<std::string> DEFAULT_HTML;
    static const std::unordered_map<std::string, int> DEFAULT_HTML_TAG;
    // 将16进制数转换为10进制数
    static int ConverHex(char ch);  
};

#endif