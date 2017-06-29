#pragma once

enum REQUEST_TYPE {RAW, PARSED, SELECT, INSERT, COMPLETE};

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <exception>

struct Select_Data {
    std::string query;
    std::vector<std::pair<int, int> > col_meta_data; 
};

struct Insert_Data {
    std::vector<std::string> insert_queries;
};

class BadRequest : std::exception {
private:
    std::string m_error_msg;

public:
    BadRequest(const char * const error_msg) : m_error_msg(error_msg) {}
    virtual ~BadRequest() {}

    virtual const char* what() const noexcept { return m_error_msg.c_str(); }
};

class Request {
public:
    virtual ~Request() {} 
    
    // EFFECTS:
    // Implemented by derived classes - it will return
    // whatever type of handler this is
    virtual REQUEST_TYPE get_type() = 0;
};

// A complete type request that
// is more or less just a placeholder
// so that we can tell services once
// a request has been fully processed
class CompleteRequest : public Request {
public:
    virtual ~CompleteRequest() {}

    virtual REQUEST_TYPE get_type() { return COMPLETE; }
};

class TextRequest : public Request {
private:
    std::string m_data;

public:
    TextRequest(const std::string & buffer) : m_data(buffer) {}
    virtual ~TextRequest() {}

    virtual REQUEST_TYPE get_type() { return RAW; }

    const std::string & get_data() const { return m_data; }
};

class ParsedParamsRequest : public Request {
private:
    std::shared_ptr<const std::unordered_map<std::string
    , std::string> > m_params_to_vals;

public:
    ParsedParamsRequest(
        std::shared_ptr<const std::unordered_map<std::string
        , std::string> > data) 
    : m_params_to_vals(data) {}
    
    virtual ~ParsedParamsRequest() {}

    virtual REQUEST_TYPE get_type() { return PARSED; }

    std::shared_ptr<const std::unordered_map<std::string
    , std::string> > get_params() const { return m_params_to_vals; }
};

// For user data that is going to be used
// to query a set of keys and their associated
// values in other tables
class SelectRequest : public Request {
private:
    Select_Data m_data;

public:
    SelectRequest(const std::string & table_name
        , const std::unordered_map<std::string, std::pair<int, int> > * cols
        , const std::vector<std::string> * keys
        , std::shared_ptr<const std::unordered_map<std::string
            , std::string> > data);
    
    virtual ~SelectRequest() {}

    const Select_Data & get_data() const { return m_data; }

    virtual REQUEST_TYPE get_type() { return SELECT; }
};


// For user data that is going to be inserted
// into another table
class InsertRequest : public Request {
private:
    Insert_Data m_data;

public:
    InsertRequest(const std::string & table_name
        , std::shared_ptr<const std::vector<std::unordered_map<std::string
            , std::string> > > data);
    
    virtual ~InsertRequest() {}

    const Insert_Data & get_data() const { return m_data; }

    virtual REQUEST_TYPE get_type() { return INSERT; }
};
