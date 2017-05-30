#include "Common.h"
#include "Logger.h"

#include <sstream>
#include <cmath>
#include <vector>

bool NaturalOrderLess::operator() (const std::string & x, const std::string & y) const {
	// the shorter string is always considered less
	// digit cases:
	// only when the first characters currently being
	// compared in both strings is a digit
	// when a sequence of numbers is found and is equal
	// the resulting number is compared and the smaller number
	// makes the string that it belongs to less than the other
	// character cases:
	// the smaller ascii value that is found first makes the
	// string that holds this the string less than the other
	// Equal cases:
	// This must be false, as this function will only return
	// true for x < y, same as the comparator std::less does

	for (std::string::const_iterator i(x.begin()), j(y.begin()); i != x.end(); ++i, ++j) {
		if (j == y.end()) {
			return false;
		}

		if (std::isdigit((int)*i) != 0 && std::isdigit((int)*j) != 0) {
			// digits case
			// first find the resulting
			// number from the sequence
			int decimal_multiplier = 1;
			int val_x = *i - '0';
			int val_y = *j - '0';
			++i, ++j;

			for (; i != x.end(); ++i, ++j) {
				if (j == y.end()) {
					return false;
				}
				if (std::isdigit((int)*i) != 0 && std::isdigit((int)*j) != 0) {
					(val_x *= std::pow(10, decimal_multiplier)) += (int)(*i - '0');
					(val_y *= std::pow(10, decimal_multiplier)) += (int)(*j - '0');
				} else if (std::isdigit(*i) == 0 || std::isdigit(*j) == 0) {
					break;
				}

				++decimal_multiplier;
			}


			if (i == x.end() && j != y.end()) {
				return true;
			} else if (i == x.end() && j == y.end()) {
				if (val_x < val_y) {
					return true;
				} else if (val_x > val_y) {
					return false;
				}

				break;
			} else {
				if (std::isdigit((int)*j) != 0 && std::isdigit((int)*i) == 0) {
					return true;
				} else if (std::isdigit((int)*i) != 0 && std::isdigit((int)*j) == 0) {
					return false;
				} else {
					if (val_x < val_y) {
						return true;
					} else if (val_x > val_y) {
						return false;
					}
				}
			}
		} else {
			// character case

			if (*i < *j) {
				return true;
			} else if (*i > *j) {
				return false;
			}
		}
	}

	return false;
}

DatabaseInterface::DatabaseInterface(std::string & conn_str) : 
 m_max_reconnects(3), m_conn_str(conn_str) 
{
    if (!reconnect()) {
    	BOOST_LOG_TRIVIAL(error) << "We are unable to connect to database. Cannot execute this query.";
    }
}

bool DatabaseInterface::reconnect() {
	int attempts = 0;
	while (attempts < m_max_reconnects) {
		try {
			m_db.reset(new SimpleDB::Database(m_conn_str));
			return true;
		}
		catch (SimpleDB::Exception & e) {
			++attempts;
			continue;
		}
	}

	return false;
}

bool DatabaseInterface::run_query(std::string & query) {

}

std::string url_decode(const std::string & str) {
	std::string ret;
	char ch;
	int i, ii, len = str.length();

	for (i = 0; i < len; i++) {
		if (str[i] != '%') {
			if (str[i] == '+')
				ret += ' ';
			else
				ret += str[i];
		} else {
			sscanf(str.substr(i + 1, 2).c_str(), "%x", &ii);
			ch = static_cast<char>(ii);
			ret += ch;
			i = i + 2;
		}
	}
	return ret;
}

std::string format_aisles_output(std::map<std::string, std::vector<std::string>, NaturalOrderLess > & aisles) {
	std::ostringstream output;

	for (std::map<std::string, std::vector<std::string> >::iterator i = aisles.begin(); i != aisles.end(); ++i) {
		output << i->first << ": ";
		for (std::vector<std::string>::iterator j = i->second.begin(); j != i->second.end(); ++j) {
			output << *j << " ";
		}
		output << "\n\n";
	}

	return output.str();
}