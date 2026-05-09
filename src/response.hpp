#pragma once
#include <string>
#include <map>

struct Response {
    int status_code;
    std::string status_text;
    std::map<std::string, std::string> headers;
    std::string body;

    std::string to_string() const;
};

Response make_200();
Response make_201();
Response make_200_with_body(const std::string& body);
Response make_200_with_body_gzip(const std::string& body);
Response make_404();
