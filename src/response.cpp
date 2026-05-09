#include "response.hpp"

std::string Response::to_string() const {
    std::string result = "HTTP/1.1 " + std::to_string(status_code) + " " + status_text + "\r\n";
    for (const auto& [key, value] : headers)
        result += key + ": " + value + "\r\n";
    result += "\r\n";
    result += body;
    return result;
}
Response make_200_with_body(const std::string& body){
    return {200, "OK", {{"Content-Type", "text/plain"}, {"Content-Length", std::to_string(body.size())}}, body};
}

Response make_200_with_body_gzip(const std::string& body){
    return {200, "OK", {{"Content-Type", "text/plain"},{"Content-Encoding", "gzip"}, {"Content-Length", std::to_string(body.size())}}, body};
}


Response make_200() {
    return {200, "OK", {}, ""};
}

Response make_201() {
    return {201, "Created", {}, ""};
}

Response make_404() {
    return {404, "Not Found", {}, ""};
}
