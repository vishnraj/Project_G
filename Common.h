#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>

#include "SimpleDB/SimpleDB.h"

// Comparator for natural order sort
struct NaturalOrderLess {
	bool operator() (const std::string & x, const std::string & y) const; 
};

class DatabaseInterface {
    const int m_max_reconnects; 

	std::string m_conn_str;
	std::unique_ptr<SimpleDB::Database> m_db;
    
    bool reconnect();

public:
	DatabaseInterface(std::string & conn_str);

    bool run_query(std::string & query);
};

// List order in the order that is present in the map of aisles
std::string format_aisles_output(std::map<std::string, std::vector<std::string>, NaturalOrderLess > & aisles);

// Replaces encoded url values for certain characters
// with the original character
std::string url_decode(const std::string & str);