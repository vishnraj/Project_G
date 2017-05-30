#include "Logger.h"

#include <boost/log/core.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/file.hpp>

#include <string>

namespace logging = boost::log;

namespace Logger {
    void init_logging(const char * log_dir, const char * level) {
        logging::register_simple_formatter_factory< logging::trivial::severity_level, char >("Severity");
        logging::add_common_attributes();
        
        logging::add_file_log(
            log_dir,
            logging::keywords::auto_flush = true, 
            logging::keywords::format = "[%TimeStamp%] [%Severity%]: %Message%"
        );

        std::string min_severity(level);
        if (min_severity == "--debug") {
            logging::core::get()->set_filter(logging::trivial::severity >= logging::trivial::debug);
        } else {
            logging::core::get()->set_filter(logging::trivial::severity >= logging::trivial::info);
        }  
    }
}
