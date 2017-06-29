#pragma once

#include <string>

class HttpServer {
private:
    int m_backlog{0};
    int m_buffer_length{0};
    bool m_closed{true};
    std::string m_port;
    std::string m_buffer;

    int m_sock_fd;
    int m_new_fd;
    struct addrinfo * m_res;

    void create_connection();
    void cleanup();
public:
    HttpServer(const char * port, int backlog, int buffer_length);
    ~HttpServer();
 
    inline std::string get_buffer() {
        return m_buffer;
    }

    void receive_request();
    void send_response(const std::string & message);
};