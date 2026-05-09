#pragma once
#include "request.hpp"
#include "response.hpp"

Response route(const Request& req, const std::string &directory_path);
Response handle_directory_route(const std::string &filename, const std::string &directory_path);
Response handle_file_write(const std::string &filename, const std::string &directory_path, const std::string &body);
Response handle_compression(const std::string &body);