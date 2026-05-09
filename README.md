# HTTP Server in C++

A from-scratch HTTP/1.1 server built in C++, No frameworks, no abstractions — raw POSIX sockets, manual HTTP parsing, and zlib compression.

---

## Features

- HTTP/1.1 compliant responses
- Persistent connections (`Connection: keep-alive` / `Connection: close`)
- Concurrent connections via thread pool (fixed worker threads, task queue)
- gzip compression via zlib (`Accept-Encoding: gzip`)
- Multiple compression scheme negotiation
- File serving (`GET /files/{name}`)
- File upload (`POST /files/{name}`)
- Request timing logs per thread

---

## Architecture

```
src/
├── main.cpp            — Entry point: parses CLI args, accept loop, enqueues clients
├── server.cpp/hpp      — Socket setup: create, bind, listen, accept
├── request.cpp/hpp     — HTTP request parser: method, path, headers, body
├── response.cpp/hpp    — HTTP response builder: status codes, headers, body
├── router.cpp/hpp      — Request routing: maps paths to handlers
└── threadpool.cpp/hpp  — Thread pool: fixed workers, mutex-protected task queue
```

### Request Lifecycle

```
Client connects (TCP 3-way handshake)
    └── accept() returns client_fd
        └── pool.enqueue(client_fd) — added to task queue
            └── idle worker thread picks it up
                └── while(true)
                ├── recv() — read raw bytes
                ├── parse_request() — extract method, path, headers, body
                ├── route() — match path to handler
                │     ├── /              → 200 OK
                │     ├── /echo/{str}    → 200 + body (optionally gzip compressed)
                │     ├── /user-agent    → 200 + User-Agent header value as body
                │     ├── GET /files/{}  → 200 + file contents or 404
                │     └── POST /files/{} → write body to file → 201 Created
                ├── res.to_string() — serialize response
                ├── send() — write bytes to client
                └── break if Connection: close
```

---

## What I Learned

### Networking & Sockets
- How TCP connections work — the 3-way handshake, `socket()`, `bind()`, `listen()`, `accept()`
- Difference between `SOCK_STREAM` (TCP) and `SOCK_DGRAM` (UDP)
- Why `SO_REUSEADDR` matters — avoids "Address already in use" on server restart
- What a file descriptor is and why `recv()`/`send()` use them
- How `bytes_read <= 0` signals disconnect vs error

### HTTP/1.1 Protocol
- HTTP is just plain text over TCP — request line, headers, blank line, body
- The structure of an HTTP request: `METHOD /path HTTP/version\r\nHeader: Value\r\n\r\nbody`
- Why `\r\n` is used instead of just `\n` (CRLF — the HTTP spec requires it)
- Difference between HTTP/1.0 (close after each request) and HTTP/1.1 (persistent by default)
- How `Connection: close` signals a clean shutdown from either side
- Status codes: 200 OK, 201 Created, 404 Not Found

### Concurrency
- Why a single-threaded server blocks on one slow client
- Why unbounded thread-per-client doesn't scale — 10k clients = 10k threads, huge memory overhead
- Thread pool pattern — pre-spawn N workers, queue incoming connections, idle threads sleep on a condition variable
- How `std::condition_variable` works — workers block on `cv.wait()`, `notify_one()` wakes exactly one idle worker
- Why the task queue needs a `std::mutex` — multiple threads reading/writing the queue is a race condition
- Why `client_fd` and `directory_path` must be captured by value in the lambda — the outer loop moves on
- Read-only shared state (like `directory_path`) is safe without locks — race conditions only happen on writes

### gzip Compression
- How `deflateInit2()` with `windowBits = 15 + 16` switches from raw deflate to gzip format
- Why `compressBound()` exists — compressed output can be larger than input for small payloads
- What `z_stream` is — zlib's state struct for streaming compression
- How to verify gzip output: `1f 8b` magic bytes, CRC32 checksum, original size in footer
- `Content-Encoding: gzip` + `Content-Length` must reflect compressed size, not original

### C++ & Systems
- Using `std::istringstream` to tokenize the HTTP request line
- Binary file I/O with `std::ifstream`/`std::ofstream` and `std::ios::binary`
- `std::chrono::steady_clock` for measuring request processing time
- Why `std::string(buffer, bytes_read)` is safer than treating the buffer as a C string

---

## Running Locally

**Prerequisites:** CMake, a C++23 compiler, vcpkg, zlib

```sh
# Build
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake
cmake --build ./build

# Run
./build/http-server --directory /path/to/files/
```

Or use the convenience script:
```sh
./your_program.sh --directory /path/to/files/
```

---

## Testing

```sh
# Basic response
curl -v http://localhost:4221/

# Echo
curl http://localhost:4221/echo/hello

# User-Agent
curl http://localhost:4221/user-agent

# gzip compression
curl -v -H "Accept-Encoding: gzip" http://localhost:4221/echo/abc | hexdump -C

# File read
curl http://localhost:4221/files/foo

# File write
curl -X POST -d "Hello, World!" http://localhost:4221/files/foo

# Persistent connection
curl --http1.1 -v http://localhost:4221/echo/first --next http://localhost:4221/echo/second

# Connection: close
curl -v -H "Connection: close" http://localhost:4221/echo/hello

# Concurrent connections
curl http://localhost:4221/echo/hello & curl http://localhost:4221/echo/world &
```

---

## Build System

CMake collects all `.cpp`/`.hpp` files from `src/` automatically — any new file added there is compiled without touching `CMakeLists.txt`.

```cmake
file(GLOB_RECURSE SOURCE_FILES src/*.cpp src/*.hpp)
```

Links against `pthreads` (for `std::thread`) and `zlib` (for gzip compression).