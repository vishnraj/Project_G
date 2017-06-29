#include "Requests.h"

#include <sstream>

#include "Logger.h"

SelectRequest::SelectRequest(const std::string & table_name
        , const std::unordered_map<std::string, std::pair<int, int> > * cols
        , const std::vector<std::string> * keys
        , std::shared_ptr<const std::unordered_map<std::string
            , std::string> > data) 
{
    {
        std::ostringstream query;
        std::vector<std::string>::const_iterator itr(keys->begin()), itr_end(keys->end());
        std::unordered_map<std::string
            , std::string>::const_iterator val_itr = data->find(*itr); 

        if (val_itr == data->end()) {
            BOOST_LOG_TRIVIAL(error) << "We had an error with creating a select request.";
            throw BadRequest("Failed to create select request.");
        }

        query << "SELECT * FROM " << table_name << " WHERE (" << *itr << "=\'" << val_itr->second << "\'";
        ++itr;
        for (; itr != itr_end; ++itr) {
            val_itr = data->find(*itr);
            if (val_itr == data->end()) {
                BOOST_LOG_TRIVIAL(error) << "We had an error with creating a select request. Key was not found.";
                throw BadRequest("Failed to create select request.");
            }

            query << " AND " << *itr << "=\'" << val_itr->second << "\'";
        }
        query << ");";

        m_data.query = query.str();
    }

    {
        // We do this because when we query with keys,
        // we desired all other associated columns that
        // correspond to the values of those keys
        std::unordered_map<std::string
        , std::pair<int, int> >::const_iterator itr(cols->begin()), itr_end(cols->end());

        for (; itr != itr_end; ++itr) {
            m_data.col_meta_data.push_back(itr->second);
        }
    }
}

InsertRequest::InsertRequest(const std::string & table_name
        , std::shared_ptr<const std::vector<std::unordered_map<std::string
            , std::string> > > data)
{ 
    std::vector<std::unordered_map<std::string
        , std::string> >::const_iterator itr(data->begin()), itr_end(data->end());

    for (; itr != itr_end; ++itr) {
        std::ostringstream query;
        std::unordered_map<std::string
            , std::string>::const_iterator jtr(itr->begin()), jtr_end(itr->end());

        query << "INSERT INTO " << table_name << " (" << jtr->first;
        ++jtr;
        for (; jtr != jtr_end; ++jtr) {
            query << ", " << jtr->first;
        }
        query << ")";

        jtr = itr->begin();
        query << " VALUES(\'" << jtr->second << "\'";
        ++jtr;
        for (; jtr != jtr_end; ++jtr) {
            query << ", \'" << jtr->second << "\'";
        }
        query << ")";

        m_data.insert_queries.push_back(query.str());
    } 
}