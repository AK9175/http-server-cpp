#include <iostream>
#include <string>
#include <chrono>
#include <unistd.h>
#include <sys/socket.h>
#include <thread>
#include "server.hpp"
#include "request.hpp"
#include "router.hpp"
#include "threadpool.hpp"

int main(int argc, char **argv) {
    // Flush after every std::cout / std::cerr
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    std::string directory_path = "";
    for(int i = 1 ; i < argc-1 ; i++){
      if(std::string(argv[i]) == "--directory"){
        directory_path = argv[i+1];
        break;
      }
    }
    std::cout << "Logs from your program will appear here!\n";

    // Create server socket, bind to port 4221, and start listening
    int server_fd = create_server(4221);
    if (server_fd < 0) return 1;

    // Pre-spawn a fixed number of worker threads
    ThreadPool pool(4);

    // Define how each worker handles a client connection
    pool.client_handler = [directory_path](int client_fd) {
        while(true){
          // Read the raw HTTP request from the client into buffer
          char buffer[4096];
          ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer), 0);

          // bytes_read == 0: client disconnected, -1: recv failed
          if (bytes_read <= 0) break;

          // Parse raw bytes into a structured Request (method, path, headers, body)
          std::string raw(buffer, bytes_read);
          Request req = parse_request(raw);

          // Route the request to the appropriate handler and get a Response
          auto start = std::chrono::steady_clock::now();
          Response res = route(req, directory_path);
          auto end = std::chrono::steady_clock::now();
          auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
          std::cout << "[thread " << std::this_thread::get_id() << "] "
                    << req.method << " " << req.path
                    << " -> " << res.status_code
                    << " (" << duration << "us)\n";

          // Serialize the Response to an HTTP string and send it back
          bool should_close = req.headers.count("Connection") && req.headers.at("Connection") == "close";
          if(should_close) res.headers["Connection"] = "close";
          std::string response_str = res.to_string();
          send(client_fd, response_str.c_str(), response_str.size(), 0);

          if(should_close) break;
        }
        close(client_fd);
    };

    std::cout << "Waiting for clients to connect...\n";

    while(true){
      // Block until a client connects, then hand off to the thread pool
      int client_fd = accept_client(server_fd);
      std::cout << "Client connected\n";
      pool.enqueue(client_fd);
    }

    close(server_fd);
    return 0;
}
