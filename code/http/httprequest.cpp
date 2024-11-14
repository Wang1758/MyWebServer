#include "httprequest.h"
using namespace std;

const unordered_set<string> HttpRequest::DEFAULT_HTML{
    "/index",
    "/register",
    "/login",
    "/welcome",
    "/video",
    "/picture",
};

const unordered_map<string, int> HttpRequest::DEFAULT_HTML_TAG {
    {"/register.html", 0},
    {"/login.html", 1},
};

void HttpRequest::Init() {
    method_ = path_ = version_ = body_ = "";
    state_ = REQUEST_LINE;
    header_.clear();
    post_.clear();
}

bool HttpRequest::IsKeepAlive() const {
    if (header_.count("Connection") == 1)
    {
        return header_.find("Connection")->second == "keep-alive" && version_ == "1.1";
    }
    return false;
}

bool HttpRequest::parse(Buffer& buff) {
    const char CRLF[] = "\r\n"; // 行结束标志
    if(buff.ReadableBytes()<=0){
        return false;
    }

    // 不断读取请求中的每一行
    while(buff.ReadableBytes()&&state_ != FINISH){
        const char* lineEnd = search(buff.Peek(), buff.BeginWriteConst(), CRLF, CRLF+2);
        std::string line(buff.Peek(), lineEnd);
        // 根据当前解析状态，对当前行做不同的处理，并转换状态
        std::cout<< line<< std::endl;
        switch(state_){
        case REQUEST_LINE:
            if(!ParseRequestLine_(line)) {
                std::cout << "这一行出错了！" << std::endl;                                                                 // debug
                return false;
            }
            ParsePath_();
            break;
        case HEADERS:
            ParseHeader_(line);
            // 剩余可读区只剩下两个字符，说明读取到了最后，并且该请求是一个get请求，没有请求体
            if(buff.ReadableBytes() <= 2) {
                state_ = FINISH;
                std::cout<< "下一个请求"<<std::endl;
            }
            break;
        case BODY:
            ParseBody_(line);
            std::cout << "下一个请求" << std::endl;
            break;
        default:
            break;
        }
        // 如果所有数据读完了
        if(lineEnd == buff.BeginWrite()) {
            break;
        }
        buff.RetrieveUntil(lineEnd+2);  // 跳过当前行的回车换行符，到下一行的开始
    }
    LOG_DEBUG("[%s], [%s], [%s]", method_.c_str(), path_.c_str(), version_.c_str());
    return true;
}

void HttpRequest::ParsePath_(){
    // 如果是根路径，则转换为默认首页
    if(path_=="/") {
        path_ = "/index.html";
    } else {    // 否则匹配其他情况
        for(auto &item:DEFAULT_HTML) {
            if(item == path_) {
                path_ += ".html";
                break;
            }
        }
    }
}

bool HttpRequest::ParseRequestLine_(const string& line) {
    regex patten("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$"); // 定义正则表达式模式
    smatch subMatch;    // 定义samtch对象存储匹配结果
    if(regex_match(line, subMatch, patten)) {
        method_ = subMatch[1];
        path_ = subMatch[2];
        version_ = subMatch[3];
        state_ = HEADERS;
        return true;
    }
    LOG_ERROR("RequestLine Error");
    return false;
}

void HttpRequest::ParseHeader_(const string& line) {
    regex patten("^([^:]*): ?(.*)$");
    smatch subMatch;
    if(regex_match(line, subMatch, patten)) {
        header_[subMatch[1]] = subMatch[2];
    } else {
        state_ = BODY;
    }
}

void HttpRequest::ParseBody_(const string& line){
    body_ = line;
    ParsePost_();
    state_ = FINISH;
    LOG_DEBUG("Body:%s, len:%d", line.c_str(), line.size());
}

int HttpRequest::ConverHex(char ch) {
    if(ch >='A'&& ch<='F'){
        return ch -'A'+10;
    }
    if(ch >= 'a'&&ch <= 'f'){
        return ch -'a' +10;
    }
    if(ch >= '0' && ch <='9'){
        return ch ='0';
    }
    LOG_ERROR("http请求中的请求体的值不是16进制数");
    return ch;
}

void HttpRequest::ParsePost_() {
    if (method_ == "POST" && header_["Content-Type"] == "application/x-www-form-urlencoded"){
        ParseFromUrlencode_();
        if(DEFAULT_HTML_TAG.count(path_)) {
            int tag = DEFAULT_HTML_TAG.find(path_)->second;
            LOG_DEBUG("Tag:%d", tag);
            if(tag == 0 || tag == 1) {
                bool isLogin = (tag == 1);
                if(UserVerify(post_["username"], post_["password"], isLogin)) {
                    path_ = "/welcome.html";
                } else {
                    path_ = "/error.html";
                }
            }
        }
    }
}

void HttpRequest::ParseFromUrlencode_() {
    if(body_.size() == 0){return;}

    string key,value;
    int num = 0;
    int n = body_.size();
    int i = 0, j = 0;

    for(; i < n; i++) {
        char ch = body_[i];
        switch(ch) {
        case '=':   // =前面的是key，后面的一直到&之前是value,
            key = body_.substr(j,i-j);
            j = i+1;
            break;
        case '+':   // 空格字符被转换为+
            body_[i] = ' ';
            break;
        case '%':   // 特殊字符被转换为%后面跟上其ASCII码值的十六进制表示
            num = ConverHex(body_[i+1])*16 + ConverHex(body_[i+2]);
            body_[i+2] = num%10 +'0';
            body_[i+1] = num/10 + '0';
            i+=2;
            break;
        case '&':   // 下一个键值对
            value = body_.substr(j,i-j);
            j = i+1;
            post_[key] = value;
            LOG_DEBUG("%s = %s", key.c_str(), value.c_str());
            break;
        default:    // 跳过普通字符
            break;
        }
    }

    assert(j<=i);
    // 这里还需要再检查一次，因为最后一个键值对不会跟上’&’
    if(post_.count(key) == 0 && j <i){
        value = body_.substr(j, i-j);
        post_[key] = value;
    }
}

bool HttpRequest::UserVerify(const string &name, const string &pwd, bool isLogin)
{
    if (name == "" || pwd == "")
    {
        return false;
    }
    LOG_INFO("Verify name:%s pwd:%s", name.c_str(), pwd.c_str());
    MYSQL *sql;
    SqlConnRAII(&sql, SqlConnPool::Instance());
    assert(sql);

    bool flag = false;
    unsigned int j = 0;
    (void) j;
    char order[256] = {0};
    MYSQL_FIELD *fields = nullptr;
    MYSQL_RES *res = nullptr;

    if (!isLogin)
    {
        flag = true;
    }
    /* 查询用户及密码 */
    snprintf(order, 256, "SELECT username, password FROM user WHERE username='%s' LIMIT 1", name.c_str()); // 末尾会补上\0
    LOG_DEBUG("%s", order);

    if (mysql_query(sql, order))
    { // 返回值为0表示查询成功
        mysql_free_result(res);
        return false;
    }
    res = mysql_store_result(sql);    // 针对select，将数据一次性加载到内存
    j = mysql_num_fields(res);        // 获取结果集中的列的数量
    fields = mysql_fetch_fields(res); // 获取结果集中所有列的元数据（字段名、类型、长度等信息）
    (void) fields;

    while (MYSQL_ROW row = mysql_fetch_row(res))
    { // 获取一行的数据，MYSQL_ROW是一个char**，每一行分别存储一个字段的值
        LOG_DEBUG("MYSQL ROW: %s %s", row[0], row[1]);
        string password(row[1]);
        // 如果是登录，检查密码是否正确
        if (isLogin)
        {
            if (pwd == password)
            {
                flag = true;
            }
            else
            {
                flag = false;
                LOG_DEBUG("pwd error!");
            }
        }
        else
        { // 否则是注册请求，但是已经查出了用户名，说明该用户名被占用
            flag = false;
            LOG_DEBUG("user used!");
        }
    }
    mysql_free_result(res);

    /* 注册行为 且 用户名未被使用*/
    if (!isLogin && flag == true)
    {
        LOG_DEBUG("regirster!");
        bzero(order, 256);
        snprintf(order, 256, "INSERT INTO user(username, password) VALUES('%s','%s')", name.c_str(), pwd.c_str());
        LOG_DEBUG("%s", order);
        if (mysql_query(sql, order))
        {
            LOG_DEBUG("Insert error!");
            flag = false;
        }
        flag = true;
    }
    SqlConnPool::Instance()->FreeConn(sql);
    LOG_DEBUG("UserVerify success!!");
    return flag;
}

std::string HttpRequest::path() const
{
    return path_;
}

std::string &HttpRequest::path()
{
    return path_;
}
std::string HttpRequest::method() const
{
    return method_;
}

std::string HttpRequest::version() const
{
    return version_;
}

std::string HttpRequest::GetPost(const std::string &key) const
{
    assert(key != "");
    if (post_.count(key) == 1)
    {
        return post_.find(key)->second;
    }
    return "";
}

std::string HttpRequest::GetPost(const char *key) const
{
    assert(key != nullptr);
    if (post_.count(key) == 1)
    {
        return post_.find(key)->second;
    }
    return "";
}