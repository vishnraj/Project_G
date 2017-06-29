#include "HttpDBService.h"
#include "FactoryFunctions.h"

#include "Logger.h"

const char * const UNKNOWN_ISSUE = "[FAILED] We had an unknown issue with this request. Please try again later.";
const char * const FAILED_COMPLETE_REQUEST = "[FAILED] When we attempted a database transaction with your request, we had an error. Try again and/or please contact us to let us know about this issue.";

HttpDBService::HttpDBService(const char * port, int backlog
    , int buffer_length, const std::string & inital_type) :
    m_inital_type(inital_type), m_port(port), m_backlog(backlog)
    , m_buffer_length(buffer_length)
{

}

void HttpDBService::start() {
    m_server.reset(new HttpServer(m_port, m_backlog, m_buffer_length));
    while (1) {
        m_server->receive_request();

        std::shared_ptr<Request> request = create_incoming_request(
            m_inital_type, m_server->get_buffer());
       
        if (!request) {
            BOOST_LOG_TRIVIAL(error) << "An error occured when creating initial request. We cannot proceed. Exiting.";
            exit(1);
        }

        std::string response;
        bool request_error = false;
        while(request->get_type() != REQUEST_TYPE::COMPLETE &&
            (response = m_manager.get_response()).empty()) 
        {
            request = m_manager.process_request(request);

            if (!request) {
                request_error = true;
                BOOST_LOG_TRIVIAL(error) << "We had an issue creating a request. We will reset current state of request manager and continue.";
                if (!(response  = m_manager.get_response()).empty()) {
                    m_server->send_response(response);
                } else {
                    BOOST_LOG_TRIVIAL(error) << "Error with creating request did not have a response. Investigate this later.";
                    m_server->send_response(UNKNOWN_ISSUE);
                }
                m_manager.full_reset();
                break;
            }
        }

        if (request_error) {
            continue;
        }

        if (request->get_type() == REQUEST_TYPE::COMPLETE) {
            if (m_manager.get_response().empty()) {
                response = FAILED_COMPLETE_REQUEST;
                BOOST_LOG_TRIVIAL(error) << "A complete type request did not set a response. Investigate this later.";
                m_manager.full_reset();
            } else {
                response = m_manager.get_response();
            }
        } else {
            // this means there was an error encountered
            // so we must reset the state of the manager
            m_manager.full_reset();
        }

        m_server->send_response(response);
        m_manager.reset_response();
    }
}