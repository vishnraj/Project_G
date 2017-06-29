#include <iostream>

#include <string>
#include <memory>

#include "../HttpDBService.h"
#include "../Handlers.h"
#include "../AisleHandlerFuncs.h"

const char * const PORT = "8037";                   // the port users will be connecting to
const char * const INITIAL_REQUEST_TYPE = "TEXT";   // the type of request sent by the client
                                                    // (currently only plain text types are supported)
const int BACKLOG = 1;                              // how many pending connections queue will hold
const int MAX_LENGTH = 3000;                        // the length of the receiving buffer

int main() {
    HttpDBService service(PORT, BACKLOG, MAX_LENGTH, INITIAL_REQUEST_TYPE);

    std::shared_ptr<RequestHandler> handler;

    TextToParamsParser * store_data_parser = new TextToParamsParser(parse_store_request);
    TextToParamsParser * aisle_data_parser = new TextToParamsParser(parse_aisle_request);

    SelectParamsValidator * store_data_validator = new SelectParamsValidator(verify_store_request);
    InsertParamsValidator * aisle_data_validator = new InsertParamsValidator(verify_aisle_request);

    SelectHandler * store_data_selector = new SelectHandler(select_store_data);
    InsertHandler * aisle_data_inserter = new InsertHandler(insert_aisle_data);
    
    handler.reset(static_cast<RequestHandler *>(store_data_parser));
    service.add_handler(handler);
    handler.reset(static_cast<RequestHandler *>(aisle_data_parser));
    service.add_handler(handler);
    handler.reset(static_cast<RequestHandler *>(store_data_validator));
    service.add_handler(handler);
    handler.reset(static_cast<RequestHandler *>(aisle_data_validator));
    service.add_handler(handler);
    handler.reset(static_cast<RequestHandler *>(store_data_selector));
    service.add_handler(handler);
    handler.reset(static_cast<RequestHandler *>(aisle_data_inserter));
    service.add_handler(handler);
    
    service.start();
}