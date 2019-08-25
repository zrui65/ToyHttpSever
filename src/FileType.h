
#pragma once

#include <map>
#include <string>

class FileType {
public:
    FileType() {
        _filetype[".html"] = "text/html";
        _filetype[".htm"] = "text/html";
        _filetype["default"] = "text/html";
        _filetype[".c"] = "text/plain";
        _filetype[".txt"] = "text/plain";
        _filetype[".ico"] = "image/x-icon";
        _filetype[".jpg"] = "image/jpeg";
        _filetype[".bmp"] = "image/bmp";
        _filetype[".png"] = "image/png";
        _filetype[".gif"] = "image/gif";
        _filetype[".doc"] = "application/msword";
        _filetype[".gz"] = "application/x-gzip";
        _filetype[".mp3"] = "audio/mp3";
        _filetype[".avi"] = "video/x-msvideo";
    }

    std::string getFileType(const std::string& file) {
      return _filetype[file];
    }

private:
    std::map<std::string, std::string> _filetype;
};

