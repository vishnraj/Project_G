#include "AisleDatabaseCommon.h"

/*HACK - done to share data between requests*/ 

bool G_data_exists = false;
int64_t G_sid = -1;

std::string G_current_name = "";
std::string G_current_street = "";
std::string G_current_city = "";
std::string G_current_state = "";
std::string G_current_zip = "";
std::string G_current_country = "";

/********************************************/

const char * const G_store_table_name = "AisleItemLocator.Store";
const char * const G_aisle_table_name = "AisleItemLocator.Aisle";
const char * const G_status_table_name = "AisleItemLocator.Status";

std::unique_ptr<ODBC::ODBCInterface> G_conn;

std::unordered_map<std::string, std::pair<int, int> > G_store_cols = {
    {"Name", std::pair<int, int>(ODBC::COL_ID::STRING, 35)},
    {"Street", std::pair<int, int>(ODBC::COL_ID::STRING, 35)},
    {"City", std::pair<int, int>(ODBC::COL_ID::STRING, 35)},
    {"State", std::pair<int, int>(ODBC::COL_ID::STRING, 35)},
    {"Zip", std::pair<int, int>(ODBC::COL_ID::STRING, 35)},
    {"Country", std::pair<int, int>(ODBC::COL_ID::STRING, 35)},
    {"SID", std::pair<int, int>(ODBC::COL_ID::LONG_INT, 20)},
};
std::unordered_map<std::string, std::pair<int, int> > G_aisle_cols = {
    {"Items", std::pair<int, int>(ODBC::COL_ID::STRING, 255)},
    {"SID", std::pair<int, int>(ODBC::COL_ID::LONG_INT, 20)},
    {"Symbol", std::pair<int, int>(ODBC::COL_ID::STRING, 10)}
};
std::unordered_map<std::string, std::pair<int, int> > G_status_cols = {
    {"Threshold", std::pair<int, int>(ODBC::COL_ID::DOUBLE, -1)},
    {"SID", std::pair<int, int>(ODBC::COL_ID::LONG_INT, 20)},
    {"UpVote", std::pair<int, int>(ODBC::COL_ID::LONG_INT, 20)},
    {"DownVote", std::pair<int, int>(ODBC::COL_ID::LONG_INT, 20)},
    {"FromStore", std::pair<int, int>(ODBC::COL_ID::BOOL, -1)}
};

std::vector<std::string> G_store_pkeys = {"Name", "Street", "City", "State", "Zip", "Country"};
std::vector<std::string> G_aisle_pkeys = {"SID", "Symbol"};
std::vector<std::string> G_status_pkeys = {"SID"};

void reset_global_aisle_data() {
    G_data_exists = false;
    G_sid = -1;
    G_current_name = "";
    G_current_street = "";
    G_current_city = "";
    G_current_state = "";
    G_current_zip = "";
    G_current_country = "";
}