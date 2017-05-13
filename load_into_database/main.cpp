#include <cstdio>
#include <string.h>
#include <cstdlib>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <dirent.h>

#include <iostream>
#include <iomanip>
#include <map>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <cmath>

#include "../SimpleDB/SimpleDB.h"

// C-style constants
#define MYPORT "8037"  // the port users will be connecting to
#define BACKLOG 1     // how many pending connections queue will hold
#define MAX_LENGTH 3000 // the length of the receiving buffer

// C++-style constants
const std::string a_bunch_of_dashes("----------------------------\n");
const int MAX_ROWS = 30;
const int MAX_COLUMNS = 15;

struct natural_order_less {
	bool operator() (const std::string & x, const std::string & y) const {
		// the shorter string is always considered less
		// digit cases:
		// only when the first characters currently being
		// compared in both string is a digit
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
};

// Global state variables
std::map<std::string, std::vector <std::string>, natural_order_less> G_aisles; // This will be unordered when loaded
																			   // and will be sorted when retrieved instead

std::string G_store_name;
std::string G_address;
bool G_can_insert = false;
std::string G_response;
std::string G_conn_string("DSN=AisleItemLocator;UID=vishnraj;PWD=***REMOVED***");
SimpleDB::Database G_db(G_conn_string, SimpleDB::Database::CONNECTION_METHOD_SQLDriverConnect);

// Utility functions
std::string format_aisles_output();
std::string urlDecode(const std::string & str);

// Aisle data request helper functions
void parse_csv(const std::string & aisle_info);
void read_aisle_info(std::string & aisle_info, std::istringstream & payload);
bool is_csv(std::string filename);
void load_to_db();

// Key data request helper functions
bool key_data_in_payload(std::string & payload, int start_pos);
bool update_open();

// Request handling helper functions
void handle_incoming_aisle_data(std::string & payload, std::istringstream & request, int start_pos);
void handle_incoming_key_data(std::string & payload, int start_pos);

// Client-Server functions
void handle_request(ssize_t & ret, char * buffer, bool & closed);
void send_response(int new_fd, std::string message);

// Socket intialization functions
void cleanup(int & new_fd, int & sockfd, struct addrinfo * res);
int instantiate_connection(int & sock_fd);
void setup(struct addrinfo & hints, struct addrinfo * res, int & sockfd, int & new_fd);

std::string urlDecode(const std::string & str) {
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

std::string format_aisles_output() {
	std::ostringstream output;

	for (std::map<std::string, std::vector<std::string> >::iterator i = G_aisles.begin(); i != G_aisles.end(); ++i) {
		output << i->first << ": ";
		for (std::vector<std::string>::iterator j = i->second.begin(); j != i->second.end(); ++j) {
			output << *j << " ";
		}
		output << "\n\n";
	}

	return output.str();
}

void load_to_db() {
	// loading operations
	std::string street;
	std::string city;
	std::string state;
	std::string zip;
	std::string country;

	int start_pos = 0;
	int end_pos = G_address.find(',', start_pos);
	street = G_address.substr(start_pos, end_pos);
	
	start_pos = end_pos + 1;
	end_pos = G_address.find(',', start_pos);
	city = G_address.substr(start_pos, end_pos - start_pos);
	
	start_pos = end_pos + 1;
	end_pos = G_address.find(' ', start_pos);
	start_pos = end_pos + 1;
	end_pos = G_address.find(' ', start_pos);
	state = G_address.substr(start_pos, end_pos - start_pos);
	
	start_pos = end_pos + 1;
	end_pos = G_address.find(',', start_pos);
	zip = G_address.substr(start_pos, end_pos - start_pos);
	
	start_pos = end_pos;
	country = G_address.substr(start_pos + 1, std::string::npos);

	std::string::iterator start_itr = std::remove(street.begin(), street.end(), ' ');
	street.erase(start_itr, street.end());
	
	start_itr = std::remove(city.begin(), city.end(), ' ');
	city.erase(start_itr, city.end());
	
	start_itr = std::remove(state.begin(), state.end(), ' ');
	state.erase(start_itr, state.end());
	
	start_itr = std::remove(zip.begin(), zip.end(), ' ');
	zip.erase(start_itr, zip.end());
	
	start_itr = std::remove(country.begin(), country.end(), ' ');
	country.erase(start_itr, country.end());

	start_itr = std::remove(country.begin(), country.end(), ' ');
	country.erase(start_itr, country.end());

	start_itr = std::remove(G_store_name.begin(), G_store_name.end(), ' ');
	G_store_name.erase(start_itr, G_store_name.end());

	std::cout << street << '\n';
	std::cout << city << '\n';
	std::cout << state << '\n';
	std::cout << zip << '\n';
	std::cout << country << '\n';
	std::cout << G_store_name << '\n';

	//SimpleDB::ScopedTransaction transaction(G_db);

	// within this section we will check whether or
	// not the store data already exists in the table
	// and select it if it does or insert and then
	// select it so we can retrieve the SID for it

	//transaction.commit();
}

void parse_csv(const std::string & aisle_info) {
	std::istringstream iss(aisle_info);
	std::string line;
	int num_rows = 1;
	while (getline(iss, line)) {
		// frontend should explain that spaces should not be to indicate a difference in aisle symbol
		std::string::iterator start_itr = std::remove(line.begin(), line.end(), ' ');
		line.erase(start_itr, line.end());

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
					G_aisles[aisle_symbol];
					first = false;
				} else {
					G_aisles[aisle_symbol].push_back(item);
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
			if (num_columns == MAX_COLUMNS) {
				std::cerr << "We have received a file containig >= " << num_columns << " columns which is beyong the supported number for a store, which is " << MAX_COLUMNS << ". We are cutting off here.";
				break; // protect against spamming us with an impossibly
					   // large number of rows for a grocery store
			}
		}

		++num_rows;
		if (num_rows == MAX_ROWS) {
			std::cerr << "We have received a file containig >= " << num_rows << " rows which is beyong the supported number for a store, which is " << MAX_ROWS << ". We are cutting off here.";
			break; // protect against spamming us an impossibly
				   // large number of rows for a grocery store
		}
	}
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

bool is_csv(std::string filename) {
	int pos = filename.find(".csv");

	if (pos == std::string::npos) {
		return false;
	}

	return true;
}

void handle_incoming_aisle_data(std::string & payload, std::istringstream & request, int start_pos) {
	std::string filename = payload.substr(start_pos, std::string::npos);
	if (is_csv(filename)) {
		std::string aisle_info;
		// the rest of the payload appears two lines
		// after the the payload containing filename
		// in the headers that we are currently sending
		std::getline(request, payload); std::getline(request, payload);

		read_aisle_info(aisle_info, request);
		parse_csv(aisle_info);

		if (G_aisles.size() != 0 && G_can_insert) {
			G_response = "We have received aisle information for " + G_store_name + ":\n\n" + format_aisles_output() + "and will now proceed to load it.";
			load_to_db();
		} else {
			G_response = "We have failed to receive aisle info for " + G_store_name + " or we have received it, but we are unable to load it because it is currently consistent.";
		}
	} else {
		G_response = "We have received file for store " + G_store_name + ", however it is not a csv file. Please format data as a comma separated csv file and resend it. Thank you.";
	}

	// End end of a load transaction
	// we are clearing everything for
	// the next set of requests
	G_aisles.clear();
	G_store_name.clear();
	G_address.clear();
	G_can_insert = false;
}

bool update_open() {
	// check the status of this store's data
	// to see whether or no it can updated

	return true;
}

bool key_data_in_payload(std::string & payload, int start_pos) {
	// store name is already confirmed
	// from our initial search of the request
	// we shouldn't have to search for it here

	start_pos = payload.find("=", start_pos);
	int end_pos = payload.find("&", start_pos);
	int len = end_pos - start_pos;
	G_store_name = payload.substr(start_pos + 1, len - 1);
	G_store_name = urlDecode(G_store_name);
	
	std::cout << G_store_name << std::endl;

	if (G_store_name.empty())
		return false;

	start_pos = payload.find("address");

	if (start_pos == std::string::npos)
		return false;

	start_pos = payload.find("=", start_pos);
	end_pos = payload.find("&", start_pos);
	len = end_pos - start_pos;
	G_address = payload.substr(start_pos + 1, len - 1);
	G_address = urlDecode(G_address);
	
	std::cout << G_address << std::endl;

	if (G_address.empty())
		return false;

	return true;
}

void handle_incoming_key_data(std::string & payload, int start_pos) {
	bool is_key_data_in_payload = key_data_in_payload(payload, start_pos);
	bool can_be_updated = update_open();

	if (is_key_data_in_payload && can_be_updated) {
		G_response = "We have succeeded in verifying that the store name " + G_store_name + " with address " + G_address + " can be updated!";
		G_can_insert = true;
	} else if (!is_key_data_in_payload) {
		G_response = "Request did not contain either a store name or store address. Please resubmit your request with these fields.";
	} else if (!can_be_updated) {
		G_response = G_store_name + "'s data is currently closed for updates. If you find that the data is not accurate currently, submit a vote to our accuracy service for data innaccuracy.";
	}
}

void handle_request(ssize_t & ret, char * buffer, bool & closed) {
	if (ret > 0) {
		std::istringstream request(std::string(buffer, ret));
		std::cout << "Request sent to service:\n" << request.str() << std::endl;
		
		std::string line;
		while (std::getline(request, line)) {
			int pos(0);
			if ((pos = line.find("store_name")) != std::string::npos) {
				// handles first request, which contains the key data for database
				handle_incoming_key_data(line, pos);
				break;
			} else if ((pos = line.find("filename")) != std::string::npos) {
				// handles second request, which contains the aisle data to be inserted
				handle_incoming_aisle_data(line, request, pos);
				break;
			}

		}
	} else if (ret == 0) {
		std::cout << "Client closed connection. Terminating connection.\n" << a_bunch_of_dashes << std::endl;
		closed = true;
	} else {
		std::cout << "We had an invalid request.\n" << a_bunch_of_dashes << std::endl;
	}
}

void send_response(int new_fd, std::string message) {
	std::ostringstream response;
	response << "HTTP/1.0 200 OK\r\nContent-Length: " << message.size() << "\r\nContent-Type: text/html\r\n\r\n" << message;
	int ret = send(new_fd, response.str().c_str(), response.str().length(), 0);
	if (ret == -1) {
		std::cout << "Failed to send." << std::endl;
		exit(EXIT_FAILURE);
	}

	std::cout << "Sent data. Length was " << ret << " bytes. Response length was supposed to be " << response.str().length() << " bytes.\n";
	std::cout << "Response:\n" << message << std::endl;
}

void cleanup(int & new_fd, int & sockfd, struct addrinfo * res) {
	close(sockfd);
	close(new_fd);
	freeaddrinfo(res); // free the linked list
}

int instantiate_connection(int & sock_fd) {
	struct sockaddr_storage their_addr;
	if (listen(sock_fd, BACKLOG) == -1) {
		std::cout << "Failed to listen." << std::endl;
		return 1;
	}

	std::cout << "Listening for connection.\n";

	// now accept an incoming connection:

	socklen_t addr_size = sizeof their_addr;
	int new_fd = accept(sock_fd, (struct sockaddr *)&their_addr, &addr_size);

	if (new_fd == -1) {
		std::cout << "Failed to accept." << std::endl;
		return -1;
	}

	std::cout << "Accepting. Closing connection to listening socket.\n";

	return new_fd;
}

void setup(struct addrinfo & hints, struct addrinfo * res, int & sockfd, int & new_fd) {
	// !! don't forget your error checking for these calls !!

	// first, load up address structs with getaddrinfo():

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;  // use IPv4 or IPv6, whichever
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;     // fill in my IP for me

	getaddrinfo(NULL, MYPORT, &hints, &res);

	// make a socket, bind it, and listen on it:

	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

	if (bind(sockfd, res->ai_addr, res->ai_addrlen) == -1) {
		std::cout << "Failed to bind." << std::endl;
		exit(EXIT_FAILURE);
	}

	std::cout << "Bound.\n";

	new_fd = instantiate_connection(sockfd);
	if (new_fd == -1) {
		exit(EXIT_FAILURE);
	}
}

int main(int argc, char *argv[])
{
	struct addrinfo hints, *res;
	int sockfd, new_fd;
	setup(hints, res, sockfd, new_fd);
	
	bool closed = false;
	while (true) {
		char buffer[MAX_LENGTH];
		ssize_t ret = recv(new_fd, buffer, MAX_LENGTH, 0);
		handle_request(ret, buffer, closed);

		if (!closed) {
			send_response(new_fd, G_response);
		} else {
			close(new_fd);
			new_fd = instantiate_connection(sockfd);
			if (new_fd == -1) {
				std::cout << "Failed to acquire file handle for accepting new connections. Cleaning up and ending this run of application." << std::endl;
				break; // we cannot accept new connections
					   // so we will terminate here
			}

			closed = false;
		}
	}

	cleanup(new_fd, sockfd, res);
	return 0;
}