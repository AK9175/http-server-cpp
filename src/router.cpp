#include <fstream>
#include <sstream>
#include <iostream>
#include <zlib.h>
#include "router.hpp"

Response route(const Request& req, const std::string& directory_path) {
    if (req.path == "/")
        return make_200();
    else if(req.path.find("/echo/") == 0){
        bool isgzip = req.headers.count("Accept-Encoding") && req.headers.at("Accept-Encoding").find("gzip") != std::string::npos;
        return isgzip ? handle_compression(req.path.substr(6)) : make_200_with_body(req.path.substr(6));
    }
    else if(req.path == "/user-agent"){
        return make_200_with_body(req.headers.count("User-Agent") ? req.headers.at("User-Agent") : "");
    }
    else if(req.path.find("/files/") == 0 && req.method == "GET"){
        return handle_directory_route(req.path.substr(7), directory_path);
    }
    else if(req.path.find("/files/") == 0 && req.method == "POST"){
        return handle_file_write(req.path.substr(7), directory_path, req.body);
    }
    return make_404();
}

Response handle_file_write(const std::string &filename, const std::string &directory_path, const std::string &body){
    std::string filepath = directory_path + filename;
    std::ofstream file(filepath, std::ios::binary);
    if(!file.is_open()){
        return make_404();
    }
    file << body;
    return make_201();
}

Response handle_directory_route(const std::string &filename, const std::string &directory_path){
    std::string filepath = directory_path + filename;
    std::ifstream file(filepath, std::ios::binary);
    if(!file.is_open()){
        return make_404();
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    return make_200_with_body(ss.str());
}

Response handle_compression(const std::string &body){
    //Size of the compressed object
    uLongf compressed_size = compressBound(body.size()) + 18;
    //Allocates the output buffer with null bytes
    std::string compressed(compressed_size, '\0');

    z_stream stream{};
    deflateInit2(&stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 15+16, 8, Z_DEFAULT_STRATEGY);
    stream.avail_in = body.size(); // how many bytes are left to read
    stream.next_in = (Bytef*)body.data(); // pointer to input data
    stream.avail_out = compressed_size; // how much space is left
    stream.next_out = (Bytef*)compressed.data(); // pointer to output buffer
    deflate(&stream, Z_FINISH); //Actual compression
    deflateEnd(&stream);
    compressed.resize(stream.total_out);
    return make_200_with_body_gzip(compressed);
}