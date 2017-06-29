#include "RequestManager.h"

#include "Logger.h"

void RequestManager::add_handler(std::shared_ptr<RequestHandler> handler) {
    switch (handler->get_type()) {
        case PARSER :
            {
                m_handlers[RAW].push_back(handler);
                m_max_handlers[RAW]++;
            }
            break;
        case VALIDATOR :
            {
                m_handlers[PARSED].push_back(handler);
                m_max_handlers[PARSED]++;
            }
            break;
        case SELECTOR :
            {
                m_handlers[SELECT].push_back(handler);
                m_max_handlers[SELECT]++;
            }
            break;
        case INSERTER :
            {
                m_handlers[INSERT].push_back(handler);
                m_max_handlers[INSERT]++;
            }
            break;
        default :
            BOOST_LOG_TRIVIAL(error) << "Unknown handler type " << handler->get_type() << ".";
            return;
    }

    BOOST_LOG_TRIVIAL(info) << "Handler successfully added!";
}

std::shared_ptr<Request> RequestManager::process_request(
    std::shared_ptr<Request> request) 
{
    REQUEST_TYPE incoming_type = request->get_type();
    std::shared_ptr<RequestHandler> handler = m_handlers[incoming_type][m_current_handler[incoming_type]];
    
    request = handler->handle_request(request);

    if (!handler->get_response().empty()) {
        m_response = handler->get_response();
        handler->reset_response();
    }

    if (m_max_handlers[incoming_type] != 1) {
        if (m_current_handler[incoming_type] < (m_max_handlers[incoming_type] - 1)) {
            m_current_handler[incoming_type]++;
        } else {
            m_current_handler[incoming_type] = 0;
        }    
    }

    return request;
}

 void RequestManager::full_reset() {
    std::unordered_map<int
        , int>::iterator itr(m_current_handler.begin()), itr_end(m_current_handler.end());
    
    for (; itr != itr_end; ++itr) {
        itr->second = 0;
    }   
}