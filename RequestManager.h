#pragma once

#include "Requests.h"
#include "Handlers.h"

#include <unordered_map>
#include <vector>
#include <memory>

// Note:
// This manages the chain of responsibility
// by moving the request to each handler in
// the chain, until a response has been
// set in a given handler, at which point
// the manager will set its own response to
// notify the service that it is time
// to send a response back to the client
// (response for the handler should be reset
// here and for this object response should be
// reset by caller of process_request, so that
// for the next client request, we will be in
// a valid state)
class RequestManager {
private:
    std::string m_response;

    // Keeps request type to request handler
    std::unordered_map<int, std::vector<std::shared_ptr<RequestHandler> > > m_handlers;
    std::unordered_map<int, int> m_max_handlers; // number of handlers per type
    std::unordered_map<int, int> m_current_handler; // current handler being used per type
public:
    RequestManager() {}
    ~RequestManager() {}

    // EFFECTS:
    // Adds handlers
    void add_handler(std::shared_ptr<RequestHandler> handler);
    // EFFECTS:
    // process the incoming sub_request (a step in the processing
    // of the overall client request) according to its type
    std::shared_ptr<Request> process_request(
        std::shared_ptr<Request> request);

    inline std::string get_response() const { return m_response; }
    inline void reset_response() { m_response = ""; }
    void full_reset();
};