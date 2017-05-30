#pragma once

// This library requires us to define
// dynamic linking before we declare the
// header file, so we will do this here
// and include it in other places
// to make this more organized

#define BOOST_LOG_DYN_LINK 1

#include <boost/log/trivial.hpp>

namespace Logger {
    void init_logging(const char * log_dir, const char * level);
}
