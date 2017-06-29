#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>

enum HANDLER_TYPE {PARSER, VALIDATOR, INSERTER, SELECTOR};

class Request;

class RequestHandler {
protected:
    std::string m_response;

public:
    RequestHandler() {}
    virtual ~RequestHandler() {}

    // EFFECTS:
    // Depending on the type of handler, this may or may
    // not set a response 
    virtual std::shared_ptr<Request> handle_request(
        std::shared_ptr<Request> request) = 0;
    
    virtual HANDLER_TYPE get_type() = 0;

    inline std::string get_response() const { return m_response; }
    inline void reset_response() { m_response = ""; }
};

class TextToParamsParser : public RequestHandler {
private:
    std::function<void(const std::string &
        , std::shared_ptr<std::unordered_map<std::string
            , std::string> >
        , std::string &)> m_parse_func;

public:
    TextToParamsParser(
        std::function<void(const std::string &
        , std::shared_ptr<std::unordered_map<std::string
            , std::string> >
        , std::string &)> func) : 
        m_parse_func(func) {}

    virtual ~TextToParamsParser() {}

    virtual std::shared_ptr<Request> handle_request(
        std::shared_ptr<Request> request);

    virtual HANDLER_TYPE get_type() { return PARSER; } 
};

class SelectParamsValidator : public RequestHandler {
private:
    std::function<bool(
        std::shared_ptr<const std::unordered_map<std::string
            , std::string> >
        , const std::unordered_map<std::string, std::pair<int, int> > *&
        , const std::vector<std::string> *&
        , std::string &, std::string &)> m_validate_func;

public:
    SelectParamsValidator(
        std::function<bool(
            std::shared_ptr<const std::unordered_map<std::string
            , std::string> >
            , const std::unordered_map<std::string, std::pair<int, int> > *&
            , const std::vector<std::string> *&
            , std::string &, std::string &
        )> func
    ) : m_validate_func(func) {}

    virtual ~SelectParamsValidator() {}

    virtual std::shared_ptr<Request> handle_request(
        std::shared_ptr<Request> request);

    virtual HANDLER_TYPE get_type() { return VALIDATOR; }
};

class InsertParamsValidator : public RequestHandler {
private:
    std::function<bool(
        std::shared_ptr<std::vector<std::unordered_map<std::string
            , std::string> > >
        , std::shared_ptr<const std::unordered_map<std::string
            , std::string> >
        , const std::unordered_map<std::string, std::pair<int, int> > *&
        , std::string &, std::string &
    )> m_validate_func;

public:
    InsertParamsValidator(
        std::function<bool(
            std::shared_ptr<std::vector<std::unordered_map<std::string
                , std::string> > >
            , std::shared_ptr<const std::unordered_map<std::string
                , std::string> >
            , const std::unordered_map<std::string, std::pair<int, int> > *&
            , std::string &, std::string &
        )> func
    ) : m_validate_func(func) {}

    virtual ~InsertParamsValidator() {}

    virtual std::shared_ptr<Request> handle_request(
        std::shared_ptr<Request> request);

    virtual HANDLER_TYPE get_type() { return VALIDATOR; }
};

class SelectHandler : public RequestHandler {
private:
    std::function<void(const std::string &
        , const std::vector<std::pair<int, int> > &
        , std::string &)> m_db_func;

public:
    SelectHandler(
        std::function<void(
            const std::string &
            , const std::vector<std::pair<int, int> > &
            , std::string &
        )> func
    ) : m_db_func(func) {}

    virtual ~SelectHandler() {}

    virtual std::shared_ptr<Request> handle_request(
        std::shared_ptr<Request> request);

    virtual HANDLER_TYPE get_type() { return SELECTOR; }
};

class InsertHandler : public RequestHandler {
private:
    std::function<void(
        const std::vector<std::string> insert_queries
        , std::string &)> m_db_func;

public:
    InsertHandler(
        std::function<void(
            const std::vector<std::string> insert_queries
            , std::string &
        )> func
    ) : m_db_func(func) {}

    virtual ~InsertHandler() {}

    virtual std::shared_ptr<Request> handle_request(
        std::shared_ptr<Request> request);

    virtual HANDLER_TYPE get_type() { return INSERTER; }
};
