#include "AisleHandlerFuncs.h"

#include <sstream>

#include "AisleDatabaseCommon.h"
#include "Common.h"
#include "Logger.h"

const char * const FAILED_STORE_DATA_PARSE = "[FAILED] This request did not contain the required parameters to allow us to find the desired store. Check the request and try again.";
const char * const FAILED_AISLE_DATA_PARSE = "[FAILED] This request did not contain the required parameters to allow us to insert the store data. Check the request and try again.";

const char * const FAILED_STORE_DATA_VERIFY = "[FAILED] This request did not contain all the required parameters to allow us to verify whether our records contain the desired store. Check the request and try again.";
const char * const FAILED_TO_ADD_NEW_STORE_DATA = "[FAILED] During verification we noticed a store was sent. We attempted to add data for it, but failed. Please try again later/contact us.";
const char * const FAILED_NO_AISLE_DATA_FOUND = "[FAILED] This request did not contain any aisle data. Check the request and try again.";
const char * const FAILED_MISSING_AISLE_DATA = "[FAILED] This request was missing a symbol or items for an aisle. Check the request and try again.";

const char * const SUCCESS_NEW_STORE_DATA_RECEIVED = "[SUCCESS] This store has not been added to our records! We will proceed to add it.";
const char * const SUCCESS_TRACKED_STORE_DATA_RECEIVED = "[SUCCESS] This store exists in our records! We will next check if it is open for update and update if possible.";

const char * const FAILED_AISLE_DATA_UPDATED = "[FAILED] Aisle data has been received, but it is no longer open for update. Check back later to see if the status changes or try a different location if this store has multiple locations.";
const char * const SUCCESS_AISLE_DATA_UPDATED = "[SUCCESS] Aisle data has been updated for this store.";

const char * const FAILED_STORE_AISLES_RETURNED = "[FAILED] This store has not been added yet. Please add it and we can share it.";
const char * const SUCCESS_STORE_AISLES_RETURNED = "[SUCCESS] This store has the following aisles: \n\n";

const int MAX_FILE_ROWS = 30;
const int MAX_FILE_COLUMNS = 15;
const double THRESHOLD_VOTE_PERCENTAGE = 90.0;

/*************** Helpers *********************/

// Parsing Store Data helpers

void fill_store_request_params(const std::string & store_name
    , const std::string & address
    , std::unordered_map<std::string, std::string> & params_to_vals);

bool get_store_params(const std::string & payload
    , int start_pos
    , std::string & store_name
    , std::string & address);

// Parsing Aisle Data helpers

bool is_csv(std::string filename);
void read_aisle_info(std::string & aisle_info, std::istringstream & payload);
void parse_csv(const std::string & aisle_info
    , std::unordered_map<std::string, std::string> & params_to_vals);

bool get_aisle_params(std::string & payload
    , std::istringstream & request
    , int start_pos
    , std::unordered_map<std::string, std::string> & params_to_vals);

// Select Store Data helpers

bool check_store_exists(const std::string & query
    , const std::vector<std::pair<int, int> > & col_meta_data
    , bool & database_error);

// Select Aisle Data helpers

void retrieve_aisle_data(std::map<std::string
    , std::string, NaturalOrderLess> & aisles
    , bool & database_error);

// Insert Ailse Data helpers

bool is_open_for_update(const int64_t & sid_val
    , bool & database_error);
void load_to_status_table(int64_t & sid_val
    , bool & database_error);
void load_to_aisle_table(const int64_t & sid_val
    , bool data_exists
    , const std::vector<std::string> & col_meta_data
    , bool & database_error);
void load_to_store_table(const std::string & store_name 
    , const std::string & street_val, const std::string & city_val
    , const std::string & state_val, const std::string & zip_val
    , const std::string & country_val, const int64_t & sid_val
    , bool & database_error);

/********* Helper Funcs definitions **********/

void fill_store_request_params(const std::string & store_name
    , const std::string & address
    , std::unordered_map<std::string, std::string> & params_to_vals)
{
    std::string name;
    std::string street;
    std::string city;
    std::string state;
    std::string zip;
    std::string country;

    name = store_name;

    int start_pos = 0;
    int end_pos = address.find(',', start_pos);
    street = address.substr(start_pos, end_pos);

    start_pos = end_pos + 1;
    end_pos = address.find(',', start_pos);
    city = address.substr(start_pos, end_pos - start_pos);

    start_pos = end_pos + 1;
    end_pos = address.find(' ', start_pos);
    start_pos = end_pos + 1;
    end_pos = address.find(' ', start_pos);
    state = address.substr(start_pos, end_pos - start_pos);

    start_pos = end_pos + 1;
    end_pos = address.find(',', start_pos);
    zip = address.substr(start_pos, end_pos - start_pos);

    start_pos = end_pos;
    country = address.substr(start_pos + 1, std::string::npos);   

    // is this really necessary?
    // I would guess that spacing from
    // Google's location API is consistent

    std::string::iterator start_itr = std::remove(name.begin(), name.end(), ' ');
    name.erase(start_itr, name.end());

    start_itr = std::remove(street.begin(), street.end(), ' ');
    street.erase(start_itr, street.end());

    start_itr = std::remove(city.begin(), city.end(), ' ');
    city.erase(start_itr, city.end());

    start_itr = std::remove(state.begin(), state.end(), ' ');
    state.erase(start_itr, state.end());

    start_itr = std::remove(zip.begin(), zip.end(), ' ');
    zip.erase(start_itr, zip.end());

    start_itr = std::remove(country.begin(), country.end(), ' ');
    country.erase(start_itr, country.end());    

    BOOST_LOG_TRIVIAL(debug) << name;
    BOOST_LOG_TRIVIAL(debug) << street;
    BOOST_LOG_TRIVIAL(debug) << city;
    BOOST_LOG_TRIVIAL(debug) << state;
    BOOST_LOG_TRIVIAL(debug) << zip;
    BOOST_LOG_TRIVIAL(debug) << country;

    params_to_vals["Name"] = name;
    params_to_vals["Street"] = street;
    params_to_vals["City"] = city;
    params_to_vals["State"] = state;
    params_to_vals["Zip"] = zip;
    params_to_vals["Country"] = country;

    /*HACK - done to share data between requests*/

    G_current_name = name;
    G_current_street = street;
    G_current_city = city;
    G_current_state = state;
    G_current_zip = zip;
    G_current_country = country;
}

bool get_store_params(const std::string & payload
    , int start_pos
    , std::string & store_name
    , std::string & address) {
    // store name is already confirmed
    // from our initial search of the request
    // we shouldn't have to search for it here

    start_pos = payload.find("=", start_pos);
    int end_pos = payload.find("&", start_pos);
    int len = end_pos - start_pos;
    store_name = payload.substr(start_pos + 1, len - 1);
    store_name = url_decode(store_name);
    
    BOOST_LOG_TRIVIAL(debug) << store_name;

    if (store_name.empty())
        return false;

    start_pos = payload.find("address");

    if (start_pos == std::string::npos)
        return false;

    start_pos = payload.find("=", start_pos);
    end_pos = payload.find("&", start_pos);
    len = end_pos - start_pos;
    address = payload.substr(start_pos + 1, len - 1);
    address = url_decode(address);
    
    BOOST_LOG_TRIVIAL(debug) << address;

    if (address.empty())
        return false;

    return true;
}

bool is_csv(std::string filename) {
    int pos = filename.find(".csv");

    if (pos == std::string::npos) {
        return false;
    }

    return true;
}

void read_aisle_info(std::string & aisle_info, std::istringstream & payload) {
    std::string line;
    while (std::getline(payload, line)) {
        if (line[0] != '-') {
            aisle_info += line + '\n';
        } else {
            break;
        }
    }
}

void parse_csv(const std::string & aisle_info
    , std::unordered_map<std::string, std::string> & params_to_vals) {
    std::istringstream iss(aisle_info);
    std::string line;
    int num_rows = 1;
    while (getline(iss, line)) {
        // frontend should explain that spaces should not be to indicate a difference in aisle symbol
        // std::string::iterator start_itr = std::remove(line.begin(), line.end(), ' ');
        // line.erase(start_itr, line.end());

        std::size_t start_pos = 0;
        std::size_t pos = line.find(",");
        bool more_columns = true;
        bool first = true;
        bool last = false;
        std::string aisle_symbol;
        int num_columns = 1;
        while (more_columns) {
            if (std::isspace(line[start_pos]) == 0 && line[start_pos] != '\0' && line[start_pos] != ',') {
                int len = pos - start_pos;
                std::string item = line.substr(start_pos, len);

                if (first) {
                    aisle_symbol = item;
                    params_to_vals[aisle_symbol];

                    first = false;
                } else {
                    params_to_vals[aisle_symbol] += item;

                    if (!first && !last) {
                        params_to_vals[aisle_symbol] += ", ";
                    }
                }

                if (last) {
                    more_columns = false;
                }

                start_pos = pos + 1;
                pos = line.find(",", start_pos);
                if (pos == std::string::npos) {
                    pos = line.size() - 1;
                    last = true;
                }
            } else {
                more_columns = false;
            }

            ++num_columns;
            if (num_columns == MAX_FILE_COLUMNS) {
                std::ostringstream error_msg;
                error_msg << "We have received a file containig >= " << num_columns << " columns which is past ";
                error_msg << MAX_FILE_COLUMNS << ". We are cutting off here."; 
                BOOST_LOG_TRIVIAL(error) << error_msg.str();
                break; // protect against spamming us with an impossibly
                       // large number of rows for a grocery store
            }
        }

        if (!aisle_symbol.empty()) {
            // in case we ended in an edge case
            // remove comma if it was inserted at the end
            if (params_to_vals[aisle_symbol].length() > 0) {
                std::string::iterator itr = params_to_vals[aisle_symbol].end() - 2;
                if (*(itr) == ',') {
                    // remove comma + space at the end
                    params_to_vals[aisle_symbol].erase(itr, itr + 1);
                }
            }    
        }
        
        ++num_rows;
        if (num_rows == MAX_FILE_ROWS) {
            std::ostringstream error_msg;
            error_msg << "We have received a file containig >= " << num_rows << " rows which is past ";
            error_msg << MAX_FILE_ROWS << ". We are cutting off here.";
            BOOST_LOG_TRIVIAL(error) << error_msg.str();
            break; // protect against spamming us an impossibly
                   // large number of rows for a grocery store
        }
    }

    for (auto aisle : params_to_vals) {
        BOOST_LOG_TRIVIAL(debug) << "In parse_csv: Aisle Symbol: " << aisle.first << " Items: " << aisle.second;
    }
}

bool get_aisle_params(std::string & payload
    , std::istringstream & request
    , int start_pos
    , std::unordered_map<std::string, std::string> & params_to_vals) 
{
    std::string filename = payload.substr(start_pos, std::string::npos);
    if (!is_csv(filename)) {
        return false;
    }

    std::string aisle_info;
    // the rest of the payload appears two lines
    // after the the payload containing filename
    // in the headers that we are currently sending
    std::getline(request, payload); std::getline(request, payload);

    read_aisle_info(aisle_info, request);
    parse_csv(aisle_info, params_to_vals);

    return true;
}

bool check_store_exists(const std::string & query
    , const std::vector<std::pair<int, int> > & col_meta_data
    , bool & database_error)
{
    BOOST_LOG_TRIVIAL(debug) << "We are checking whether the store has been added to our records.";

    std::unique_ptr<ODBC::ODBCDataSet> rows = G_conn->run_query(query, col_meta_data);
    // run_query will return a valid pointer
    // even if there are no rows -> a nullptr
    // indicates that there was some kind of
    // database error
    if (rows == nullptr) {
        BOOST_LOG_TRIVIAL(error) << "We failed to retrieve store data due to an internal database error.";
        database_error = true;
        return false;
    }

    // // this is not an error, but just an indication
    // // that this is a new store that has not been added yet
    std::weak_ptr<ODBC::ODBCRow> row = rows->next_row();
    if (!row.expired() && row.lock()->valid()) {
        // get the last column in row, as this
        // will correspond to the sid
        std::vector<SimpleDB::Column *>::const_iterator col(row.lock()->end());
        --col;

        if (*col != nullptr) {
            SimpleDB::StringColumn * sid = dynamic_cast<SimpleDB::StringColumn *>(*col);
            G_sid = std::stol(sid->value()); // long long should be 64 int on this system
            return true;
        } else {
            // this is an error, because if the store
            // has been added to our database then it
            // must have an sid
            BOOST_LOG_TRIVIAL(error) << "We failed to retrieve an SID for an already inserted store.";
            database_error = true;
            return false;
        }
    } else if (!row.expired() && !row.lock()->valid()) {
        BOOST_LOG_TRIVIAL(debug) << "Error occurred with generating columns.";
        database_error = true;
        return false;
    }

    return false; // new store, but no errors
}

void retrieve_aisle_data(std::map<std::string, std::string, NaturalOrderLess> & aisles
    , bool & database_error)
{
    BOOST_LOG_TRIVIAL(debug) << "Retrieving aisle data for store with id: " << G_sid;

    std::ostringstream query;
    query << "SELECT Symbol, Items FROM AisleItemLocator.Aisle WHERE SID=" << G_sid << ";";

    std::vector<std::pair<int, int> > col_meta_data;
    col_meta_data.push_back(G_aisle_cols["Symbol"]);
    col_meta_data.push_back(G_aisle_cols["Items"]);

    std::unique_ptr<ODBC::ODBCDataSet> rows = G_conn->run_query(query.str(), col_meta_data);

    for (std::weak_ptr<ODBC::ODBCRow> row = rows->next_row(); !row.expired(); row = rows->next_row())
    {
        if (row.lock()->valid()) {
            int count = 0;
            std::string symbol;
            std::vector<SimpleDB::Column *>::const_iterator col(row.lock()->start()), col_end(row.lock()->end());
            for (; col != col_end; ++col) {
                if (*col != nullptr) {
                    if (count == 0) {
                        SimpleDB::StringColumn * symbol_col = dynamic_cast<SimpleDB::StringColumn *>(*col);
                        symbol = symbol_col->value();
                        aisles[symbol];
                    } else if (count == 1) {
                        SimpleDB::StringColumn * items = dynamic_cast<SimpleDB::StringColumn *>(*col);
                        aisles[symbol] = items->value();
                    }
                } else {
                    BOOST_LOG_TRIVIAL(error) << "We couldn't read aisle data for a store that we have record of.";
                    database_error = true;
                    return;
                }

                ++count;
            }
        } else {
            BOOST_LOG_TRIVIAL(debug) << "Error occurred with generating columns.";
            database_error = true;
            return;
        }
    }
}

bool is_open_for_update(const int64_t & sid_val
    , bool & database_error) {
    BOOST_LOG_TRIVIAL(debug) << "Checking if we can update aisle data for SID: " << sid_val;

    std::ostringstream query;
    query << "SELECT Threshold, UpVote, DownVote, FromStore FROM AisleItemLocator.Status WHERE SID=" << sid_val << ";";

    // again, kind of sloppy, but an improvement over before
    // we can get this to potentially be more generic as well
    // (tied to some kind of database manager class)
    std::vector<std::pair<int, int> > col_meta_data;
    col_meta_data.push_back(G_status_cols["Threshold"]);
    col_meta_data.push_back(G_status_cols["UpVote"]);
    col_meta_data.push_back(G_status_cols["DownVote"]);
    col_meta_data.push_back(G_status_cols["FromStore"]);

    std::unique_ptr<ODBC::ODBCDataSet> rows = G_conn->run_query(query.str(), col_meta_data);

    std::weak_ptr<ODBC::ODBCRow> row = rows->next_row();
    if (!row.expired() && row.lock()->valid()) {
        double threshold_val = 0;
        int64_t up_vote_val = 0;
        int64_t down_vote_val = 0;
        bool from_store_val = false;

        int count = 0;
        std::vector<SimpleDB::Column *>::const_iterator col(row.lock()->start()), col_end(row.lock()->end());
        for (; col != col_end; ++col) {
            if (*col != nullptr) {
                if (count == 0) {
                    SimpleDB::DoubleColumn * threshold = dynamic_cast<SimpleDB::DoubleColumn *>(*col);
                    threshold_val = threshold->value();
                } else if (count == 1) {
                    SimpleDB::StringColumn * up_vote = dynamic_cast<SimpleDB::StringColumn *>(*col);
                    up_vote_val = std::stol(up_vote->value());
                } else if (count == 2) {
                    SimpleDB::StringColumn * down_vote = dynamic_cast<SimpleDB::StringColumn *>(*col);
                    down_vote_val = std::stol(down_vote->value());    
                } else if (count == 3) {
                    SimpleDB::BoolColumn * from_store = dynamic_cast<SimpleDB::BoolColumn *>(*col);
                    from_store_val = from_store->value();
                }
            } else {
                BOOST_LOG_TRIVIAL(error) << "We couldn't read status data for a store that we have record of.";
                database_error = true;
                return false;
            }

            ++count;
        }

        if (from_store_val) {
            return false; // store provided data should not get modified
                          // by this process, this is only for 
                          // non-store type users
        }

        int64_t total_votes = up_vote_val + down_vote_val;
        if (total_votes == 0) return true; // avoid division by 0
                                           // for case without votes

        if (threshold_val > 100 * (up_vote_val / (total_votes))) {
            return true; // the data can still be improved based on user
                         // consensus (this allows to be relatively free flowing)
        }
    } else if (!row.lock()->valid()) {
        BOOST_LOG_TRIVIAL(debug) << "Error occurred with generating columns.";
        database_error = true;
    } else {
        // we failed to generate an sid
        BOOST_LOG_TRIVIAL(error) << "We failed to read status data for a store that we have a record of.";
        database_error = true;
    }

    return false;
}

void load_to_status_table(int64_t & sid_val, bool & database_error)
{
    BOOST_LOG_TRIVIAL(debug) << "Begin loading to the status table.";

    std::ostringstream query;
    std::vector<std::pair<int, int> > col_meta_data; // used later, placeholder
                                                     // for the first query
    
    query << "INSERT INTO AisleItemLocator.Status (Threshold, UpVote, DownVote, FromStore)";
    query << "VALUES(\'" << THRESHOLD_VOTE_PERCENTAGE << "\', \'0\', \'0\', \'0\');";

    std::unique_ptr<ODBC::ODBCDataSet> rows = G_conn->run_query(query.str(), col_meta_data);
    if (rows == nullptr) {
        BOOST_LOG_TRIVIAL(error) << "We failed to load status data due to an internal database error.";
        database_error = true;
    }
    
    col_meta_data.push_back(G_status_cols["SID"]);
    query.str("");
    query << "SELECT LAST_INSERT_ID();";

    rows = G_conn->run_query(query.str(), col_meta_data);
    if (rows == nullptr) {
        BOOST_LOG_TRIVIAL(error) << "We failed to retrieve store data due to an internal database error.";
        database_error = true;
        return;
    }

    std::weak_ptr<ODBC::ODBCRow> row = rows->next_row();
    if (!row.expired() && row.lock()->valid()) {
        std::vector<SimpleDB::Column *>::const_iterator col(row.lock()->start());

        if (*col != nullptr) {
            SimpleDB::StringColumn * sid = dynamic_cast<SimpleDB::StringColumn *>(*col);
            sid_val = std::stol(sid->value()); // long long should be 64 int on this system
            BOOST_LOG_TRIVIAL(debug) << "SID generated: " << sid_val;
        } else {
            // we failed to generate an sid
            BOOST_LOG_TRIVIAL(error) << "We failed to generate an SID.";
            database_error = true;
        }
    } else if (!row.lock()->valid()) {
        BOOST_LOG_TRIVIAL(debug) << "Error occurred with generating columns.";
        database_error = true;
    } else {
        // we failed to generate an sid
        BOOST_LOG_TRIVIAL(error) << "We failed to generate an SID.";
        database_error = true;
    }
}

void load_to_aisle_table(const int64_t & sid_val
    , bool data_exists
    , const std::vector<std::string> & insert_queries
    , bool & database_error)
{
    BOOST_LOG_TRIVIAL(debug) << "Begin loading to the aisle table";

    std::vector<std::pair<int, int> > placeholder;

    if (data_exists) {
        std::ostringstream delete_query;
        delete_query << "DELETE FROM AisleItemLocator.Aisle WHERE SID=" << sid_val; // in order to maintain total
                                                                                    // accuracy, for now this will
                                                                                    // be all or nothing, but later
                                                                                    // we will change how we do this

        std::unique_ptr<ODBC::ODBCDataSet> rows = G_conn->run_query(delete_query.str(), placeholder);
        if (rows == nullptr) {
            BOOST_LOG_TRIVIAL(error) << "We failed to delete old aisle data due to an internal database error.";
            database_error = true;
            return;
        }
    }

    for (auto query : insert_queries) {
        std::unique_ptr<ODBC::ODBCDataSet> rows = G_conn->run_query(query, placeholder);
        if (rows == nullptr) {
            BOOST_LOG_TRIVIAL(error) << "We failed to load aisle data due to an internal database error.";
            database_error = true;
            return;
        }
    }
}

void load_to_store_table(const std::string & store_name 
    , const std::string & street_val, const std::string & city_val
    , const std::string & state_val, const std::string & zip_val
    , const std::string & country_val, const int64_t & sid_val
    , bool & database_error)
{
    BOOST_LOG_TRIVIAL(debug) << "Begin loading to the store table.";

    std::ostringstream query;
    query << "INSERT INTO AisleItemLocator.Store (Name, Street, City, State, Zip, Country, SID) VALUES(\'";
    query << store_name << "\', \'" << street_val << "\', \'" << city_val << "\', \'" << state_val << "\'";
    query << ", \'" << zip_val << "\', \'" << country_val << "\', \'" << sid_val << "\');";

    std::vector<std::pair<int, int> > placeholder;
    std::unique_ptr<ODBC::ODBCDataSet> rows = G_conn->run_query(query.str(), placeholder);
    if (rows == nullptr) {
        BOOST_LOG_TRIVIAL(error) << "We failed to load store data due to an internal database error.";
        database_error = true;
    }
}

/******************************************************************/

/***************** Handler Funcs defintions ***********************/

void parse_store_request(const std::string & buffer
    , std::shared_ptr<std::unordered_map<std::string, std::string> > params_to_vals
    , std::string & response) {
    std::istringstream request(buffer);
    BOOST_LOG_TRIVIAL(info) << "Received a store request. Begin parsing.";
        
    std::string line;
    while (std::getline(request, line)) {
        int pos(0);
        if ((pos = line.find("store_name")) != std::string::npos) {
            std::string store_name;
            std::string address;

            if (!get_store_params(line, pos, store_name, address)) {
                BOOST_LOG_TRIVIAL(error) << "We failed to acquire all store paramters from the request.";
                response = FAILED_STORE_DATA_PARSE;
                reset_global_aisle_data();
                return;
            }

            // also sets global aisle data, which is a bad way of doing it
            fill_store_request_params(store_name, address, *params_to_vals);
            BOOST_LOG_TRIVIAL(info) << "Success! We have acquired the store paramters from the request.";
            BOOST_LOG_TRIVIAL(info) << "Proceed to do further processing for " << store_name << " at " << address << ".";
            return;
        }     
    }

    response = FAILED_STORE_DATA_PARSE;
    reset_global_aisle_data();
}

void parse_aisle_request(const std::string & buffer
    , std::shared_ptr<std::unordered_map<std::string, std::string> > params_to_vals
    , std::string & response) {
    std::istringstream request(buffer);
    BOOST_LOG_TRIVIAL(info) << "Received an aisle request. Begin parsing.";
    
    std::string line;
    while (std::getline(request, line)) {
        int pos(0);
        if ((pos = line.find("filename")) != std::string::npos) {
            if (!get_aisle_params(line, request, pos, *params_to_vals)) {
                BOOST_LOG_TRIVIAL(error) << "We failed to acquire aisle paramters from the request.";
                response = FAILED_AISLE_DATA_PARSE;
                reset_global_aisle_data();
                return;
            }

            BOOST_LOG_TRIVIAL(info) << "Success! We have acquired the aisle paramters from the request.";
            return;
        }
    }

    response = FAILED_AISLE_DATA_PARSE;
    reset_global_aisle_data();
}

bool verify_store_request(
    std::shared_ptr<const std::unordered_map<std::string
        , std::string> > params_to_vals
    , const std::unordered_map<std::string, std::pair<int, int> > *& cols
    , const std::vector<std::string> *& keys
    , std::string & table_name, std::string & response)
{
    keys = &G_store_pkeys;
    std::vector<std::string>::const_iterator itr(keys->begin()), itr_end(keys->end());
    for (; itr != itr_end; ++itr) {
        std::unordered_map<std::string
        , std::string>::const_iterator find_itr(params_to_vals->find(*itr));

        if (find_itr == params_to_vals->end()) {
            BOOST_LOG_TRIVIAL(error) << "We failed to find all primary keys for store data in the request that was sent.";
            response = FAILED_STORE_DATA_VERIFY;
            reset_global_aisle_data();
            return false;
        }

        BOOST_LOG_TRIVIAL(debug) << "For store data, found: " << *itr;
    }

    cols = &G_store_cols;
    table_name = G_store_table_name;

    BOOST_LOG_TRIVIAL(info) << "Success! We have verified that the parameters sent for the store are part of our data model.";
    return true;
}

bool verify_aisle_request(
    std::shared_ptr<std::vector<std::unordered_map<std::string
        , std::string> > > data
    , std::shared_ptr<const std::unordered_map<std::string
        , std::string> > params_to_vals
    , const std::unordered_map<std::string, std::pair<int, int> > *& cols
    , std::string & table_name, std::string & response)
{
    if (params_to_vals->size() == 0) {
        BOOST_LOG_TRIVIAL(error) << "We failed to receive any aisle data.";
        response = FAILED_NO_AISLE_DATA_FOUND;
        return false;        
    }

    if (G_sid == -1) {
        BOOST_LOG_TRIVIAL(info) << "Loading aisle data for new store.";

        G_conn->transaction_start();

        int64_t sid;
        bool database_error = false;
        load_to_status_table(sid, database_error);

        if (database_error) {
            BOOST_LOG_TRIVIAL(error) << "Could not load status data due to database error. Rolling back.";
            response = FAILED_TO_ADD_NEW_STORE_DATA;
            reset_global_aisle_data();
            G_conn->transaction_rollback();
            return false;
        }

        load_to_store_table(G_current_name, G_current_street
            , G_current_city, G_current_state, G_current_zip
            , G_current_country, sid
            , database_error);

        if (database_error) {
            BOOST_LOG_TRIVIAL(error) << "Could not load store data due to database error. Rolling back.";
            response = FAILED_TO_ADD_NEW_STORE_DATA;
            reset_global_aisle_data();
            G_conn->transaction_rollback();
            return false;
        }

        G_conn->transaction_commit(); // everything went correctly, commit

        G_sid = sid;
    } else {
        G_data_exists = true;
    }

    std::unordered_map<std::string
    , std::string>::const_iterator itr(params_to_vals->begin()), itr_end(params_to_vals->end());

    for (; itr != itr_end; ++itr) {
        std::unordered_map<std::string, std::string> row;
        // The names for cols used here and in AisleDatabaseCommon
        // should be made configurable in the future. This is not ideal,
        // just being done in order to get this working.

        if (itr->first.empty()) {
            BOOST_LOG_TRIVIAL(error) << "Missing symbol for an aisle.";
            response = FAILED_MISSING_AISLE_DATA;
            reset_global_aisle_data();
            return false;
        }

        row["Symbol"] = itr->first;

        if (itr->second.empty()) {
            BOOST_LOG_TRIVIAL(error) << "Missing items for " << row["Symbol"];
            response = FAILED_MISSING_AISLE_DATA;
            reset_global_aisle_data();
            return false;
        }

        row["Items"] = itr->second;
        row["SID"] = std::to_string(G_sid);

        data->push_back(row);
    }

    cols = &G_aisle_cols;
    table_name = G_aisle_table_name;

    BOOST_LOG_TRIVIAL(info) << "Success! We have verified that the parameters sent for the aisles are part of our data model.";
    return true;
}

void select_store_data(
    const std::string & query
    , const std::vector<std::pair<int, int> > & col_meta_data
    , std::string & response)
{
    bool database_error = false;
    if (!check_store_exists(query, col_meta_data, database_error)
        && !database_error) {
        response = SUCCESS_NEW_STORE_DATA_RECEIVED; 
    } else if (!database_error) {
        response = SUCCESS_TRACKED_STORE_DATA_RECEIVED;
    } else {
        BOOST_LOG_TRIVIAL(error) << "Ecnountered a database error when retrieving store data.";
        reset_global_aisle_data();
    }
}

void select_store_aisles(const std::string & query
    , const std::vector<std::pair<int, int> > & col_meta_data
    , std::string & response) 
{
    bool database_error = false;
    if (!check_store_exists(query, col_meta_data, database_error)
        && !database_error) {
        BOOST_LOG_TRIVIAL(info) << "We do not have any data for this store, yet.";
        response = FAILED_STORE_AISLES_RETURNED;
        reset_global_aisle_data();
        return;
    }

    std::map<std::string, std::string, NaturalOrderLess> aisles;
    retrieve_aisle_data(aisles, database_error);

    if (!database_error) {
        BOOST_LOG_TRIVIAL(info) << "Success! We have retrieved the aisle data for the store that the user requested.";
        (response += SUCCESS_STORE_AISLES_RETURNED) += format_aisles_output(aisles);  
    }

    // should be the last function in the
    // chain of handler funcs getting called
    reset_global_aisle_data();
}

void insert_aisle_data(
    const std::vector<std::string> & insert_queries
    , std::string & response)
{ 
    // check if the store exists in our records
    bool database_error = false;
    // next we check if we can update this table
    // by querying the status table

    if (!G_data_exists) {
        G_conn->transaction_start();

        // now we load the data to the aisle table
        load_to_aisle_table(G_sid, false
            , insert_queries, database_error);

        if (database_error) {
            BOOST_LOG_TRIVIAL(error) << "Could not add aisle data for store due to database error. Rolling back.";
            reset_global_aisle_data();
            G_conn->transaction_rollback();
            return;
        }

        G_conn->transaction_commit();
    } else {
        if (is_open_for_update(G_sid, database_error)) {
            G_conn->transaction_start();

            load_to_aisle_table(G_sid, true
                , insert_queries, database_error);

            if (database_error) {
                BOOST_LOG_TRIVIAL(error) << "Could not update aisle data for store due to database error. Rolling back.";
                reset_global_aisle_data();
                G_conn->transaction_rollback();
                return;
            }

            G_conn->transaction_commit();
        } else if (!database_error) {
            BOOST_LOG_TRIVIAL(info) << "Aisle data was received, but could not be updated for this store.";
            response = FAILED_AISLE_DATA_UPDATED;
            reset_global_aisle_data();
            return;
        } else {
            // This procedure will eventually become
            // part of class function, as it is repeated
            // in many places
            BOOST_LOG_TRIVIAL(error) << "Could not update data for the store due to a database error.";
            reset_global_aisle_data();
            return;
        }    
    } 

    BOOST_LOG_TRIVIAL(info) << "Success! We have updated the data in the database.";
    response = SUCCESS_AISLE_DATA_UPDATED;
    reset_global_aisle_data();
}