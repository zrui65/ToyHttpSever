
#pragma once

#include <string>
#include <map>

#define CR '\r'
#define LF '\n'

const int64_t KEEP_ALIVE_TIME = 300000000; // 5min

class HttpRequest {
private:
    enum ParseState
    {
        STATE_PARSE_REQUEST_LINE = 0x8000,
        STATE_PARSE_HEADER_LINE,
        STATE_PROCESS_REQUEST
    };

    enum ParseRequestState
    {
        rq_start = 0x4000,
        rq_method,
        rq_spaces_before_uri,
        rq_after_slash_in_uri,
        rq_http,
        rq_http_H,
        rq_http_HT,
        rq_http_HTT,
        rq_http_HTTP,
        rq_first_major_digit,
        rq_major_digit,
        rq_first_minor_digit,
        rq_minor_digit,
        rq_spaces_after_digit,
        rq_almost_done
    };

    enum ParseHeaderState
    {
        hd_start = 0x2000,
        hd_key,
        hd_spaces_before_colon,
        hd_spaces_after_colon,
        hd_value,
        hd_cr,
        hd_crlf,
        hd_crlfcr
    };

    enum HttpMethod
    {
        METHOD_GET = 0x0800,
        METHOD_POST,
        METHOD_HEAD
    };

    enum HttpVersion
    {
        HTTP10 = 0x0400,
        HTTP11
    };

    enum HttpStatusCode
    {
        HTTP_200Ok = 200,
        HTTP_400BadRequest = 400,
        HTTP_404NotFound = 404
    };

public:
    HttpRequest()
    :   _state(STATE_PARSE_REQUEST_LINE),
        _request_state(rq_start),
        _header_state(hd_start),
        _pos(0),
        _last(0),
        _file_path("../html/"),
        _status(HTTP_200Ok),
        _keep_live(false)
        {};

    size_t process(std::string&, std::string&);
    bool isKeepLive() { return _keep_live; }

private:
    std::string getMethodStr();
    void printAll();
    void printState(ParseRequestState);
    void printState(ParseHeaderState);
    void setLast(size_t n) { this->_last = n; };

    void reset();
    bool setMethod(const std::string&);
    void setPath(std::string&&);
    void addHeader(const std::string&, const std::string&);
    std::string getStatusStr(HttpStatusCode);

    bool parseRequestLine(const std::string&);
    bool parseHeaderLine(std::string&);
    void processRequest(std::string&);
    void processRequestError(std::string&, HttpStatusCode);

    // 整个解析过程中的状态
    ParseState _state;
    // 解析请求行过程中的状态
    ParseRequestState _request_state;
    // 解析首部行过程中的状态
    ParseHeaderState _header_state;

    size_t _last_pos;  // 记录刚刚处理完的请求的尾位置
    size_t _pos;
    size_t _last;
    size_t _request_start;
    size_t _uri_start;
    size_t _http_major;
    size_t _http_minor;
    size_t _request_end;

    size_t _cur_header_key_start;
    size_t _cur_header_key_end;
    size_t _cur_header_value_start;
    size_t _cur_header_value_end;

    typedef std::map<std::string, std::string> HeaderMap;

    HttpMethod _method;
    HttpVersion _version;
    std::string _file_path;
    HeaderMap _headers;

    HttpStatusCode _status;
    bool _keep_live;

};

