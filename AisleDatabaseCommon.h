#pragma once

#include "ODBCTools.h" 

#include <vector>
#include <unordered_map>
#include <memory>

/*HACK - done to share data between requests*/ 

extern bool G_data_exists;
extern int64_t G_sid; // The current sid that will be used by the functions
                  // in this module

extern std::string G_current_name;
extern std::string G_current_street;
extern std::string G_current_city;
extern std::string G_current_state;
extern std::string G_current_zip;
extern std::string G_current_country;

/********************************************/

extern const char * const G_store_table_name;
extern const char * const G_aisle_table_name;
extern const char * const G_status_table_name;

extern std::unique_ptr<ODBC::ODBCInterface> G_conn;

extern std::unordered_map<std::string, std::pair<int, int> > G_store_cols;
extern std::unordered_map<std::string, std::pair<int, int> > G_aisle_cols;
extern std::unordered_map<std::string, std::pair<int, int> > G_status_cols;

extern std::vector<std::string> G_store_pkeys;
extern std::vector<std::string> G_aisle_pkeys;
extern std::vector<std::string> G_status_pkeys;

void reset_global_aisle_data();
