#pragma once

#include <string>
#include <vector>
#include <map>

// Comparator for natural order sort
struct NaturalOrderLess {
	bool operator() (const std::string & x, const std::string & y) const; 
};

// List order in the order that is present in the map of aisles
std::string format_aisles_output(std::map<std::string, std::vector<std::string>, NaturalOrderLess > & aisles);
std::string format_aisles_output(std::map<std::string, std::string, NaturalOrderLess> & aisles);

// Replaces encoded url values for certain characters
// with the original character
std::string url_decode(const std::string & str);