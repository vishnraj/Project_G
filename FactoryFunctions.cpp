#include "FactoryFunctions.h"

#include "Requests.h"
#include "Logger.h"

std::shared_ptr<Request> create_incoming_request(
    const std::string & type, const std::string & buffer)
{
    Request * tmp = nullptr;
    
    if (type == "TEXT") {
        tmp = static_cast<Request *>(new TextRequest(buffer)); 
    } else {
        BOOST_LOG_TRIVIAL(error) << "Unable to create request for unknown type " << type;
    }

    return std::shared_ptr<Request>(tmp);
}
