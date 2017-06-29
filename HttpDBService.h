#pragma once

#include "HttpServer.h"
#include "RequestManager.h"

#include <string>
#include <memory>

#include <boost/scoped_ptr.hpp>

// Note: This will implement a 
// Chain of Responsibility pattern
// which allows the request to pass
// through multiple handlers, before
// the final actions are taken and
// the response is sent - each handler
// should have the ability to set the
// final response, so the HttpServer
// can just pull this response from
// the RequestManager and send it
// back to the client, once the last
// final step in the chain is reached
// (or an invalid case)
class HttpDBService {
private:
    std::string m_inital_type;

    const char * m_port;
    int m_backlog;
    int m_buffer_length;
    boost::scoped_ptr<HttpServer> m_server;
    
    RequestManager m_manager;
 
public:
    HttpDBService(const char * port, int backlog
        , int buffer_length, const std::string & inital_type);
    ~HttpDBService() {}

    // EFFECTS:
    // Add handlers for requests
    inline void add_handler(std::shared_ptr<RequestHandler> handler) {
         m_manager.add_handler(handler);
    }

    void start();
};