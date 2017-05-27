#pragma once

#include <string>
#include <vector>
#include <map>

#include "../SimpleDB/SimpleDB.h"

// Comparator for natural order sort
struct NaturalOrderLess {
	bool operator() (const std::string & x, const std::string & y) const; 
};

class DatabaseInterface {
	std::string m_dsn;
	std::string m_user;
	std::string m_pwd;
	SimpleDB::Database m_db;

	DatabaseInterface();

};

// List order in the order that is present in the map of aisles
std::string format_aisles_output(std::map<std::string, std::vector<std::string>, NaturalOrderLess > & aisles);

// Replaces encoded url values for certain characters
// with the original character
std::string url_decode(const std::string & str);