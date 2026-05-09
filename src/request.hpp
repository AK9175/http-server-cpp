#pragma once
#include <string>
#include <map>

struct Request {
    std::string method;
    std::string path;
    std::string version;
    std::map<std::string, std::string> headers;
    std::string body;
};

Request parse_request(const std::string& raw);
