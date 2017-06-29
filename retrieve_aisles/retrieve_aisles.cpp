#include <iostream>

#include <string>
#include <memory>

#include "../HttpDBService.h"
#include "../Handlers.h"
#include "../AisleHandlerFuncs.h"
#include "../Logger.h"
#include "../AisleDatabaseCommon.h"

const char * usage = "retrieve_aisles <log_file> <min_severity_level, ex: --debug>";

const char * const PORT = "8038";                   // the port users will be connecting to
const char * const INITIAL_REQUEST_TYPE = "TEXT";   // the type of request sent by the client
                                                    // (currently only plain text types are supported)
const int BACKLOG = 1;                              // how many pending connections queue will hold
const int MAX_LENGTH = 3000;                        // the length of the receiving buffer
const int MAX_RECONNECTS = 5;
const int MAX_QUERY_ATTEMPTS = 2;

int main(int argc, char ** argv) {
    // will use boost options
    // for this later
    if (argc == 1) {
        std::cerr << usage << std::endl;
        return 1; 
    }

    char * log_file = argv[1];
    char * min_severity_leve1;
    if (argc == 2) {
        min_severity_leve1 = ""; // this defaults to info
    } else {
        min_severity_leve1 = argv[2];
    }

    Logger::init_logging(log_file, min_severity_leve1); 
    G_conn.reset(new ODBC::ODBCInterface("DSN=AisleItemLocator;", MAX_RECONNECTS, MAX_QUERY_ATTEMPTS));

    // also the temporary means of logging
    // we can use/write something using boost
    // that is better than this later
    BOOST_LOG_TRIVIAL(info) << "Starting Retrieve Aisle Service.";

    HttpDBService service(PORT, BACKLOG, MAX_LENGTH, INITIAL_REQUEST_TYPE);

    std::shared_ptr<RequestHandler> handler;

    TextToParamsParser * store_data_parser = new TextToParamsParser(parse_store_request);

    SelectParamsValidator * store_data_validator = new SelectParamsValidator(verify_store_request);

    SelectHandler * aisle_data_selector = new SelectHandler(select_store_aisles);
    
    handler.reset(static_cast<RequestHandler *>(store_data_parser));
    service.add_handler(handler);
    handler.reset(static_cast<RequestHandler *>(store_data_validator));
    service.add_handler(handler);
    handler.reset(static_cast<RequestHandler *>(aisle_data_selector));
    service.add_handler(handler);
    
    service.start();
}