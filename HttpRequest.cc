#include <iostream>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "HttpRequest.h"
#include "FileType.h"

FileType ft;

void HttpRequest::reset()
{
    _state = STATE_PARSE_REQUEST_LINE;
    _request_state = rq_start;
    _header_state = hd_start;

    _pos = 0;
    _last = 0;
    _request_start = 0;
    _uri_start = 0;
    _http_major = 0;
    _http_minor = 0;
    _request_end = 0;

    _cur_header_key_start = 0;
    _cur_header_key_end = 0;
    _cur_header_value_start = 0;
    _cur_header_value_end = 0;

    _file_path = "../html/";
    _headers.clear();

    _status = HTTP_200Ok;
}

bool HttpRequest::setMethod(const std::string& m) {
    if(m == "GET") {
        this->_method = METHOD_GET;
    }
    else if(m == "POST") {
        this->_method = METHOD_POST;
    }
    else if(m == "HEAD") {
        this->_method = METHOD_HEAD;
    }
    else {
        return false;
    }
    return true;
}

void HttpRequest::setPath(std::string&& m) {
    _file_path += m;
}

void HttpRequest::addHeader(const std::string& key, const std::string& value) {
    this->_headers[key] = value;
}

std::string HttpRequest::getMethodStr() {
    switch (this->_method)
    {
    case METHOD_GET:
        return "GET";
        break;

    case METHOD_POST:
        return "POST";
        break;

    case METHOD_HEAD:
        return "HEAD";
        break;

    default:
        return "NULL";
        break;
    }
}

std::string HttpRequest::getStatusStr(HttpStatusCode status) {
    switch (status)
    {
    case HTTP_200Ok:
        return "GET";
        break;

    case HTTP_400BadRequest:
        return "400 BadRequest!";
        break;

    case HTTP_404NotFound:
        return "404 NotFound!";
        break;

    default:
        return "";
        break;
    }
}

void HttpRequest::printAll() {
    std::cout << "Method: " << this->getMethodStr() << std::endl;
    std::cout << "URL: " << this->_file_path << std::endl;
    std::cout << "Version: HTTP/" << this->_http_major << "."
                             << this->_http_minor << std::endl;
    for(auto k : _headers) {
        std::cout << "**" << k.first << ":" << k.second << std::endl;
    }
}

void HttpRequest::printState(ParseRequestState state) {
    std::string s[15] = {
        "rq_start",
        "rq_method",
        "rq_spaces_before_uri",
        "rq_after_slash_in_uri",
        "rq_http",
        "rq_http_H",
        "rq_http_HT",
        "rq_http_HTT",
        "rq_http_HTTP",
        "rq_first_major_digit",
        "rq_major_digit",
        "rq_first_minor_digit",
        "rq_minor_digit",
        "rq_spaces_after_digit",
        "rq_almost_done"
    };

    std::cout << s[state - rq_start] << std::endl;
}

void HttpRequest::printState(ParseHeaderState state) {
    std::string s[8] = {
        "hd_start",
        "hd_key",
        "hd_spaces_before_colon",
        "hd_spaces_after_colon",
        "hd_value",
        "hd_cr",
        "hd_crlf",
        "hd_crlfcr"
    };

    std::cout << s[state - hd_start] << std::endl;
}

bool HttpRequest::parseRequestLine(const std::string& inBuffer) {
    if(this->_state != STATE_PARSE_REQUEST_LINE) return false;

    ParseRequestState state = this->_request_state;

    unsigned char ch;
    size_t pi, m;

    for(pi = this->_pos; pi < this->_last; pi++){
        ch = inBuffer[pi];

        // printState(state);
        switch(state) {
            case rq_start:
                this->_request_start = pi; // 记录本次解析的请求行字符串的首地址
                if(ch == CR || ch == LF)
                    break;
                if((ch < 'A' || ch > 'Z') && ch != '_')
                    return false;
                state = rq_method;
                break;

            case rq_method:
                if(ch == ' ') {
                    m = this->_request_start;
                    if(!this->setMethod(std::string(inBuffer.begin() + m,
                                               inBuffer.begin() + pi))) {
                        return false;
                    }
                    state = rq_spaces_before_uri;
                    break;
                }

                if((ch < 'A' || ch > 'Z') && ch != '_')
                    return false;
                break;

            case rq_spaces_before_uri:
                if(ch == '/') {
                    this->_uri_start = pi + 1; // 记录本次解析的请求行中"URL"字段的的首地址
                    state = rq_after_slash_in_uri;
                    break;
                }
                switch(ch) {
                    case ' ':
                        break;
                    default:
                        return false;
                }
                break;

            case rq_after_slash_in_uri:
                switch(ch) {
                    case ' ':
                        this->setPath(std::string(inBuffer.begin()+_uri_start,
                                             inBuffer.begin()+pi));
                        state = rq_http;
                        break;
                    default:
                        break;
                }
                break;

            case rq_http:
                switch(ch) {
                    case ' ':
                        break;
                    case 'H':
                        state = rq_http_H;
                        break;
                    default:
                        return false;
                }
                break;

            case rq_http_H:
                switch(ch) {
                    case 'T':
                        state = rq_http_HT;
                        break;
                    default:
                        return false;
                }
                break;

            case rq_http_HT:
                switch(ch) {
                    case 'T':
                        state = rq_http_HTT;
                        break;
                    default:
                        return false;
                }
                break;

            case rq_http_HTT:
                switch(ch) {
                    case 'P':
                        state = rq_http_HTTP;
                        break;
                    default:
                        return false;
                }
                break;

            case rq_http_HTTP:
                switch(ch) {
                    case '/':
                        state = rq_first_major_digit;
                        break;
                    default:
                        return false;
                }
                break;

            case rq_first_major_digit:
                if(ch < '1' || ch > '9')
                    return false;
                this->_http_major = ch - '0'; // 记录本次解析的请求行中HTTP主版本号(一位数)
                state = rq_major_digit;
                break;

            case rq_major_digit:
                if(ch == '.') {
                    state = rq_first_minor_digit; // 应该从这里直接跳出
                    break;
                }

                // 下面这部分应该用不到
                if(ch < '0' || ch > '9')
                    return false;
                this->_http_major = this->_http_major * 10 + ch - '0'; // 记录本次解析的请求行中HTTP主版本号(两位数)
                break;                                                     // 应该暂时不支持

            case rq_first_minor_digit:
                if(ch < '0' || ch > '9')
                    return false;
                this->_http_minor = ch - '0'; // 记录本次解析的请求行中HTTP次版本号(一位数)
                state = rq_minor_digit;
                break;

            case rq_minor_digit:
                if(ch == CR) {
                    state = rq_almost_done;
                    break;
                }
                if(ch == LF)
                    goto done;
                if(ch == ' ') {
                    state = rq_spaces_after_digit;
                    break;
                }
                if(ch < '0' || ch > '9')
                    return false;
                this->_http_minor = this->_http_minor * 10 + ch - '0'; // 记录本次解析的请求行中HTTP次版本号(两位数)
                break;

            case rq_spaces_after_digit:
                switch(ch) {
                    case ' ':
                        break;
                    case CR:
                        state = rq_almost_done;
                        break;
                    case LF:
                        goto done;
                    default:
                        return false;
                }
                break;

            case rq_almost_done:
                this->_request_end = pi - 1;
                switch(ch) {
                    case LF:
                        goto done;
                    default:
                        return false;
                }
        }
    }

    this->_pos = pi;
    this->_last_pos = this->_pos;
    this->_request_state = state;
    return true;

    done:
    this->_pos = pi + 1;
    this->_last_pos = this->_pos;
    if (this->_request_end == 0)
        this->_request_end = pi;
    this->_request_state = rq_start;
    this->_state = STATE_PARSE_HEADER_LINE;
    return true;
}

bool HttpRequest::parseHeaderLine(std::string& inBuffer) {
    if(this->_state != STATE_PARSE_HEADER_LINE) return false;

    ParseHeaderState state = this->_header_state;

    size_t pi;
    unsigned char ch;
    
    for (pi = this->_pos; pi < this->_last; pi++) {
        ch = inBuffer[pi];

        // printState(state);
        switch(state) {
            case hd_start:
                if(ch == CR || ch == LF)
                    break;
                this->_cur_header_key_start = pi;
                state = hd_key;
                break;

            case hd_key:
                if(ch == ' ') {
                    this->_cur_header_key_end = pi;
                    state = hd_spaces_before_colon; // 在冒号之前的空格
                    break;
                }
                if(ch == ':') {
                    this->_cur_header_key_end = pi;
                    state = hd_spaces_after_colon; // 在冒号之后的空格
                    break;
                }
                break;

            case hd_spaces_before_colon:
                if(ch == ' ')
                    break;
                else if(ch == ':') {
                    state = hd_spaces_after_colon;
                    break;
                }
                else
                    return false;

            case hd_spaces_after_colon:
                if (ch == ' ')
                    break;
                state = hd_value;
                this->_cur_header_value_start = pi;
                break;

            case hd_value:
                if(ch == CR) {
                    this->_cur_header_value_end = pi;
                    state = hd_cr;
                }
                if(ch == LF) {
                    this->_cur_header_value_end = pi;
                    state = hd_crlf;
                }
                break;

            case hd_cr:
                if(ch == LF) {
                    state = hd_crlf;
                    this->addHeader(std::string(inBuffer.begin()+_cur_header_key_start,
                                                inBuffer.begin()+_cur_header_key_end),
                                    std::string(inBuffer.begin()+_cur_header_value_start,
                                                inBuffer.begin()+_cur_header_value_end));
                    break;
                }
                else
                    return false;

            case hd_crlf:
                if(ch == CR)
                    state = hd_crlfcr;
                else{
                    this->_cur_header_key_start = pi;
                    state = hd_key;
                }
                break;

            case hd_crlfcr:
                switch(ch){
                    case LF:
                        goto done;
                    default:
                        return false;
                }
        }
    }
    this->_pos = pi;
    this->_last_pos = this->_pos;
    this->_header_state = state;
    return true;

    done:
    this->_pos = pi + 1;
    this->_last_pos = this->_pos;
    this->_header_state = hd_start;
    this->_state = STATE_PROCESS_REQUEST;
    
    // printAll();
    return true;
}

void HttpRequest::processRequestError(std::string& outbuffer,
                                     HttpStatusCode status) {
    std::string body_buff;
    body_buff += "<!DOCTYPE html><meta charset=\"utf-8\">";
    body_buff += "<title>真可怜!!!</title>";
    body_buff += "<body bgcolor=\"ffffff\">";
    body_buff += "<h1>" + getStatusStr(status) + "</h1>";
    body_buff += "<hr> <p style=\"font-size:100%\">Zhangrui's Toy Server </p>\n</body></html>";

    outbuffer += "HTTP/1.1 " + getStatusStr(status) + "\r\n";
    outbuffer += "Content-Type: text/html\r\n";
    outbuffer += "Connection: Close\r\n";
    outbuffer += "Content-Length: " + std::to_string(body_buff.size()) + "\r\n";
    outbuffer += "Server: Zhangrui's Toy Server\r\n";;
    outbuffer += "\r\n";
    outbuffer += body_buff;

}

void HttpRequest::processRequest(std::string& outbuffer) {

    if(_status == HTTP_400BadRequest
    || (_method != METHOD_GET && _method != METHOD_HEAD)) {
        std::cerr << "Parse request failed! send HTTP_400BadRequest" << std::endl;
        std::cerr << "Or" << std::endl;
        std::cerr << "Method is not GET or HEAD! send HTTP_400BadRequest" << std::endl;
        processRequestError(outbuffer, HTTP_400BadRequest);
    }
    else {
    
        if(_headers["Connection"] == "Keep-Alive" || _headers["Connection"] == "keep-alive") {
            _keep_live = true;
        }
        else if(_headers["Connection"] == "close") {
            _keep_live = false;
        }

        if(_file_path == "../html/test") {
            outbuffer += "HTTP/1.1 200 OK\r\n";
            outbuffer += "Content-type: text/plain\r\n";
            outbuffer += "Content-Length: 11";
            outbuffer += "\r\n\r\nHello World";
            reset();
            return;
        }

        if(_file_path == "../html/") {
            _file_path = "../html/index.html";
        }

        // std::cout << "Real _file_path = " << _file_path << std::endl;
        int target_fd = open(_file_path.c_str(), O_RDONLY, 0);
        if (target_fd < 0) {
            std::cerr << "Open " << _file_path.c_str() << " failed!!" << std::endl;
            std::cout << "      errno = " << errno << " send HTTP_404NotFound" << std::endl;
            processRequestError(outbuffer, HTTP_404NotFound);
            reset();
            return;
        }

        struct stat st;
        if(::stat(_file_path.c_str(), &st) < 0) {
            std::cerr << "stat failed!!  send HTTP_404NotFound" << std::endl;
            processRequestError(outbuffer, HTTP_404NotFound);
            reset();
            return;
        }

        outbuffer += "HTTP/1.1 200 OK\r\n";
        size_t dot = _file_path.find('.', 3);
        if(dot != _file_path.npos) {
            std::string filetype = ft.getFileType(std::string(_file_path.begin() + dot, _file_path.end()));
            outbuffer += "Content-Type: " + filetype + "\r\n";
        }

        outbuffer += "Content-Length: " + std::to_string(st.st_size) + "\r\n";
        if(_keep_live) {
            outbuffer += "Connection: Keep-Alive\r\n";
            outbuffer += "Keep-Alive: timeout=" + std::to_string(KEEP_ALIVE_TIME) + "\r\n";
        }

        outbuffer += "Server: Zhangrui's Toy Server\r\n";
        outbuffer += "\r\n";

        void *ptr = ::mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, target_fd, 0);
        ::close(target_fd);

        if (ptr == (void *)-1) {
            munmap(ptr, st.st_size);
            outbuffer.clear();
            std::cerr << "mmap failed!!  send HTTP_404NotFound" << std::endl;
            processRequestError(outbuffer, HTTP_404NotFound);
            reset();
            return;
        }
        char *src_addr = static_cast<char*>(ptr);
        outbuffer += std::string(src_addr, src_addr + st.st_size);;
        munmap(ptr, st.st_size);
    }

    reset();
}

size_t HttpRequest::process(std::string& inbuffer, std::string& outbuffer) {

    setLast(inbuffer.size());

    if(this->_state == STATE_PARSE_REQUEST_LINE ) {
        if(!this->parseRequestLine(inbuffer)) {
            // std::cerr << "[Error] parse request line" << std::endl;
            this->_status = HTTP_400BadRequest;
            this->_state = STATE_PROCESS_REQUEST;
        }
    }

    if(this->_state == STATE_PARSE_HEADER_LINE ) {
        if(!this->parseHeaderLine(inbuffer)) {
            // std::cerr << "[Error] parse header line" << std::endl;
            this->_status = HTTP_400BadRequest;
            this->_state = STATE_PROCESS_REQUEST;
        }
    }

    if(this->_state == STATE_PROCESS_REQUEST) {
        this->processRequest(outbuffer);
    }

    return this->_last_pos;
}






