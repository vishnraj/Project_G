#include "Handlers.h"

#include "Requests.h"
#include "Logger.h"

std::shared_ptr<Request> TextToParamsParser::handle_request(
        std::shared_ptr<Request> request)
{
    ParsedParamsRequest * p_request = nullptr;
    TextRequest * t_request = dynamic_cast<TextRequest *>(request.get());

    std::shared_ptr<std::unordered_map<std::string
    , std::string> > params_to_vals(new std::unordered_map<std::string
    , std::string>);

    m_parse_func(t_request->get_data(), params_to_vals, m_response);
    p_request = new ParsedParamsRequest(params_to_vals);

    return std::shared_ptr<Request>(static_cast<Request *>(p_request));
}

std::shared_ptr<Request> SelectParamsValidator::handle_request(
        std::shared_ptr<Request> request)
{
    SelectRequest * s_request = nullptr;
    ParsedParamsRequest * p_request = dynamic_cast<ParsedParamsRequest *>(request.get());

    std::shared_ptr<const std::unordered_map<std::string, std::string> > params = p_request->get_params();
    const std::unordered_map<std::string, std::pair<int, int> > * cols = nullptr;
    const std::vector<std::string> * keys = nullptr;
    std::string table_name;

    if (m_validate_func(params, cols, keys, table_name, m_response)) {
        try {
            s_request = new SelectRequest(table_name, cols, keys, params);
        } catch(std::exception & e) {
            BOOST_LOG_TRIVIAL(error) << e.what();
            s_request = nullptr;
        }    
    }

    return std::shared_ptr<Request>(static_cast<Request *>(s_request));
}

std::shared_ptr<Request> InsertParamsValidator::handle_request(
        std::shared_ptr<Request> request)
{
    InsertRequest * i_request = nullptr;
    ParsedParamsRequest * p_request = dynamic_cast<ParsedParamsRequest *>(request.get());

    std::shared_ptr<const std::unordered_map<std::string, std::string> > params = p_request->get_params();
    const std::unordered_map<std::string, std::pair<int, int> > * cols = nullptr;
    std::string table_name;
    std::shared_ptr<std::vector<std::unordered_map<std::string
        , std::string> > > insert_data(new std::vector<std::unordered_map<std::string
        , std::string> >()); // the vector of cols -> vals to be inserted

    if (m_validate_func(insert_data, params, cols, table_name, m_response)) {
        try {
            i_request = new InsertRequest(table_name, insert_data);
        } catch (std::exception & e) {
            BOOST_LOG_TRIVIAL(error) << e.what();
            i_request = nullptr;
        }
    }

    return std::shared_ptr<Request>(static_cast<Request *>(i_request));
}

std::shared_ptr<Request> SelectHandler::handle_request(
    std::shared_ptr<Request> request) {
    CompleteRequest * c_request = nullptr;
    SelectRequest * s_request = dynamic_cast<SelectRequest *>(request.get());
    
    const Select_Data & data = s_request->get_data();

    m_db_func(data.query, data.col_meta_data, m_response);

    c_request = new CompleteRequest();

    return std::shared_ptr<Request>(static_cast<Request *>(c_request));
}

std::shared_ptr<Request> InsertHandler::handle_request(
    std::shared_ptr<Request> request) {
    CompleteRequest * c_request = nullptr;
    InsertRequest * i_request = dynamic_cast<InsertRequest *>(request.get());
    
    const Insert_Data & data = i_request->get_data();

    m_db_func(data.insert_queries, m_response);

    c_request = new CompleteRequest();

    return std::shared_ptr<Request>(static_cast<Request *>(c_request));
}
