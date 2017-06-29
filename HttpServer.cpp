#include "HttpServer.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>

#include <sstream>

#include "Logger.h"

const char * SEPARATOR = "-----------------------------";

HttpServer::HttpServer(const char * port, int backlog, int buffer_length) :
    m_backlog(backlog), m_buffer_length(buffer_length), m_port(port)
{
    struct addrinfo hints;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // IPv4 or IPv6 will be supported
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // picks up the IP

    getaddrinfo(NULL, m_port.c_str(), &hints, &m_res);

    m_sock_fd = socket(m_res->ai_family, m_res->ai_socktype, m_res->ai_protocol);

    if (bind(m_sock_fd, m_res->ai_addr, m_res->ai_addrlen) == -1) {
        BOOST_LOG_TRIVIAL(error) << "Failed to bind.";
        exit(EXIT_FAILURE);
    }

    BOOST_LOG_TRIVIAL(info) << "Bound to socket. Service Initialized.\n" << SEPARATOR;
}

HttpServer::~HttpServer() {
    cleanup();
}

void HttpServer::create_connection() {
    struct sockaddr_storage their_addr;
    if (listen(m_sock_fd, m_backlog) == -1) {
        BOOST_LOG_TRIVIAL(error) << "Failed to listen. Killing application.";
        cleanup();
        exit(EXIT_FAILURE);
    }

    BOOST_LOG_TRIVIAL(info) << "Listening for connection.";

    // now accept an incoming connection:

    socklen_t addr_size = sizeof their_addr;
    m_new_fd = accept(m_sock_fd, (struct sockaddr *)&their_addr, &addr_size);

    if (m_new_fd == -1) {
        BOOST_LOG_TRIVIAL(error) << "Failed to accept. Killing application.";
        cleanup();
        exit(EXIT_FAILURE);
    }

    BOOST_LOG_TRIVIAL(info) << "Client connection accepted! Start recieving requests.";
    m_closed = false;
}

void HttpServer::cleanup() {
    close(m_sock_fd);
    close(m_new_fd);
    freeaddrinfo(m_res);
}

void HttpServer::receive_request() {
    while (1) {
        if (m_closed) {
            create_connection();           
        }

        char buffer[m_buffer_length];
        ssize_t ret = recv(m_new_fd, buffer, m_buffer_length, 0);
        BOOST_LOG_TRIVIAL(debug) << "Ret: " << ret;
        if (ret > 0) {
            m_buffer = buffer;
            BOOST_LOG_TRIVIAL(debug) << m_buffer;
            break;
        } else if (ret == 0) {
            BOOST_LOG_TRIVIAL(info) << "Client closed connection. Terminating connection.\n" << SEPARATOR;
            m_closed = true;
            close(m_new_fd); 
        } else {
            BOOST_LOG_TRIVIAL(error) << "We had an invalid request.\n" << SEPARATOR;
        }
    }
}

void HttpServer::send_response(const std::string & message) {
    std::ostringstream G_response;
    G_response << "HTTP/1.0 200 OK\r\nContent-Length: " << message.size() << "\r\nContent-Type: text/html\r\n\r\n" << message;
    ssize_t ret = send(m_new_fd, G_response.str().c_str(), G_response.str().length(), 0);
    if (ret == -1) {
        BOOST_LOG_TRIVIAL(error) << "Failed to send.";
        exit(EXIT_FAILURE);
    }

    BOOST_LOG_TRIVIAL(info) << "Sent response. Length was " << ret << " bytes. Length was supposed to be " << G_response.str().length() << " bytes.";
    BOOST_LOG_TRIVIAL(debug) << "response:\n" << message;
}