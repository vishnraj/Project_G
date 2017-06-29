#pragma once

#include <string>
#include <memory>

class Request;

std::shared_ptr<Request> create_incoming_request(
    const std::string & type, const std::string & buffer);
