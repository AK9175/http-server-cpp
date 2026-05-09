#include "request.hpp"
#include <sstream>

Request parse_request(const std::string& raw) {
    Request req;

    size_t line_end = raw.find("\r\n");
    if (line_end == std::string::npos) return req;

    // Parse request line: "GET /path HTTP/1.1"
    std::string request_line = raw.substr(0, line_end);
    std::istringstream iss(request_line);
    iss >> req.method >> req.path >> req.version;

    // Strip query string from path
    size_t question_pos = req.path.find("?");
    if (question_pos != std::string::npos)
        req.path = req.path.substr(0, question_pos);

    // Parse headers
    size_t pos = line_end + 2;
    while (pos < raw.size()) {
        size_t end = raw.find("\r\n", pos);
        if (end == std::string::npos) break;
        std::string line = raw.substr(pos, end - pos);
        if (line.empty()) {
            pos = end + 2;
            break; // end of headers
        }
        size_t colon = line.find(": ");
        if (colon != std::string::npos)
            req.headers[line.substr(0, colon)] = line.substr(colon + 2);
        pos = end + 2;
    }

    // Remaining is body
    if (pos < raw.size())
        req.body = raw.substr(pos);

    return req;
}
