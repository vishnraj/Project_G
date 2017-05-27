#include <cstdio>
#include <string.h>
#include <cstdlib>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

#include <iostream>
#include <iomanip>
#include <map>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>

#include "../SimpleDB/SimpleDB.h"

#include "Common.h"

// C-style constants
#define MYPORT "8037"  // the port users will be connecting to
#define BACKLOG 1     // how many pending connections queue will hold
#define MAX_LENGTH 3000 // the length of the receiving buffer

// C++-style constants
const char * a_bunch_of_dashes = "----------------------------\n";
const int MAX_FILE_ROWS = 30;
const int MAX_FILE_COLUMNS = 15;
const double THRESHOLD_VOTE_PERCENTAGE = 90.0;

// Global state variables
std::map<std::string, std::vector<std::string>, NaturalOrderLess> G_aisles; // sorted by aisle symbol (natural order less ftw)

std::string G_store_name;
std::string G_address;
bool G_can_insert = false;
std::string G_response;
std::string G_conn_string("DSN=AisleItemLocator;UID=vishnraj;PWD=");
SimpleDB::Database G_db(G_conn_string, SimpleDB::Database::CONNECTION_METHOD_SQLDriverConnect);

// Database loading functions
void pre_process_store_keys(std::string & street, std::string & city
	, std::string & state, std::string & zip, std::string & country);
bool check_store_exists(const std::string & street_val, const std::string & city_val
	, const std::string & state_val, const std::string & zip_val
	, const std::string & country_val, int64_t & sid_val);
bool is_open_for_update(const int64_t & sid_val);
void load_to_status_table(int64_t & sid_val);
void load_to_aisle_table(const int64_t & sid_val, bool data_exists);
void load_to_store_table(const std::string & street_val
	, const std::string & city_val, const std::string & state_val
	, const std::string & zip_val, const std::string & country_val
	, const int64_t & sid_val);
void load_to_db();

// Aisle data request helper functions
void parse_csv(const std::string & aisle_info);
void read_aisle_info(std::string & aisle_info, std::istringstream & payload);
bool is_csv(std::string filename);

// Key data request helper functions
bool key_data_in_payload(std::string & payload, int start_pos);

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

void pre_process_store_keys(std::string & street, std::string & city
	, std::string & state, std::string & zip, std::string & country)
{
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
}

bool check_store_exists(const std::string & street_val, const std::string & city_val
	, const std::string & state_val, const std::string & zip_val
	, const std::string & country_val, int64_t & sid_val)
{
	std::cout << "We are checking whether the store exists for " << G_store_name << std::endl;

	SimpleDB::StringColumn name(35);
	SimpleDB::StringColumn street(35);
	SimpleDB::StringColumn city(35);
	SimpleDB::StringColumn state(35);
	SimpleDB::StringColumn zip(35);
	SimpleDB::StringColumn country(35);
	SimpleDB::StringColumn sid(20);

	std::ostringstream query;
	query << "SELECT * FROM AisleItemLocator.Store WHERE (Name=\'" << G_store_name;
	query << "\' AND Street=\'" << street_val << "\' AND City=\'" << city_val;
	query << "\' AND State=\'" << state_val << "\' AND zip=\'" << zip_val;
	query << "\' AND Country=\'" << country_val << "\');"; // select * to get all columns,
														   // and there should only be one
														   // row given all of these are pkeys

	try {
		SimpleDB::Column *cols[7] = { &name, &street, &city, &state, &zip, &country, &sid };
		SimpleDB::Query q(G_db);
		
		q.bind(cols, 7);
		q.execute(query.str());

		if (q.fetchRow()) {
			sid_val = std::stol(sid.value()); // long long should be 64 int on this system
			return true;
		}
	} catch (SimpleDB::Exception & e) {
		std::cerr << "Caught exception in check_store_exists: " <<  e.what() << std::endl;
		G_response = "Our database had an issue while trying to verify whether or not the data for " + G_store_name + " has already been added.";
		G_response += " Try again and/or contact us to report this issue.";
		G_can_insert = false;
	}

	return false;
}

bool is_open_for_update(const int64_t & sid_val) {
	std::cout << "Checking if we can update aisle data for " << G_store_name << " with SID: " << sid_val << std::endl;

	SimpleDB::DoubleColumn threshold; // using a large varchar for double
	
	// both of these are BigInts in the table
	// but for some reason, we cannot seem to use
	// this column type to retrieve these values
	SimpleDB::StringColumn up_vote(20);
	SimpleDB::StringColumn down_vote(20);

	SimpleDB::BoolColumn from_store;
	SimpleDB::Column *cols[4] = { &threshold
								, &up_vote
								, &down_vote
								, &from_store };

	std::ostringstream query;
	query << "SELECT Threshold, UpVote, DownVote, FromStore FROM AisleItemLocator.Status WHERE SID=" << sid_val << ";";

	try {
		SimpleDB::Query q(G_db);
		q.bind(cols, 4);
		q.execute(query.str());

		if (q.fetchRow()) {
			double threshold_val = threshold.value();
			// long long should be 64 int on this system
			ino64_t up_vote_val = std::stol(up_vote.value());
			ino64_t down_vote_val = std::stol(down_vote.value());
			bool from_store_val = from_store.value();

			if (from_store_val) {
				G_response = "We have received this data from " + G_store_name + " with address " + G_address + "."; 
				G_response += " We will only update it once their records show a change.";

				return false; // store provided data should not get modified
							  // by this process, this is only for 
							  // non-store type users
			}

			ino64_t total_votes = up_vote_val + down_vote_val;
			if (total_votes == 0) return true; // avoid division by 0
											   // for case without votes

			if (threshold_val > 100 * (up_vote_val / (total_votes))) {
				return true; // the data can still be improved based on user
							 // consensus (this allows to be relatively free flowing)
			}

		} else {
			G_response = "We had a problem checking whether we could update the data for " + G_store_name + " with address " + G_address + ".";
			G_response += " Trying again and/or contact us to report this issue.";
			G_can_insert = false;
		}

	} catch (SimpleDB::Exception & e) {
		std::cerr << "Caught exception in is_open_for_update: " << e.what() << std::endl;
		G_response = "Our database had an issue while trying to verify whether or not the data for " + G_store_name + " with address " + G_address;
		G_response += " has already been added. Try again and/or contact us to report this issue.";
		G_can_insert = false;
	}

	return false;
}

void load_to_status_table(int64_t & sid_val)
{
	std::cout << "Begin loading to the status table for " << G_store_name << std::endl;
	
	SimpleDB::StringColumn sid(20); // the decimal value returned by SELECT LAST_INSERT_ID()
									// is 21 digits long, making this larger than the bytes
									// stored by BigInt, so we need this temporary conversion
									// to a string - it cannot actually be 21 digits because the
									// AutoIncrement is for a field that is 20 digits max, so 
									// 20 should work here - and it will take over 10000 years to
									// get to this point, so it should not matter
									
	SimpleDB::Column *cols[1] = { &sid };
	std::ostringstream query;

	query << "INSERT INTO AisleItemLocator.Status (Threshold, UpVote, DownVote, FromStore)";
	query << "VALUES(\'" << THRESHOLD_VOTE_PERCENTAGE << "\', \'0\', \'0\', \'0'\);";

	try {
		SimpleDB::Query q(G_db);
		q.execute(query.str());
		query.str("");
		query << "SELECT LAST_INSERT_ID();";
		q.bind(cols, 1);
		q.execute(query.str());

		if (q.fetchRow()) {
			std::string temp_val();
			sid_val = std::stol(sid.value()); // long long should be 64 int on this system
			std::cout << "SID generated: " << sid_val << " for " << G_store_name << std::endl;
		} else {
			G_response = "We were unable to add the data for " + G_store_name + " with address " + G_address + " to our records.";
			G_response += " Try agin and/or contact us if this appears to be an ongoing issue. Thank you.";
			G_can_insert = false;
		}
	} catch (SimpleDB::Exception & e) {
		std::cerr << "Caught exception in load_to_status_table: " << e.what() << std::endl;
		G_response = "Something went wrong while loading the data to our database for " + G_store_name + " with address " + G_address + ".";
		G_response += " Trying again and/or contact us. Thank you.";
		G_can_insert = false;
	}
}

void load_to_aisle_table(const int64_t & sid_val, bool data_exists)
{
	std::cout << "Begin loading to the aisle table for" << G_store_name << std::endl;

	try {
		SimpleDB::Query q(G_db);

		if (data_exists) {
			std::ostringstream query;
			query << "DELETE FROM AisleItemLocator.Aisle WHERE SID=" << sid_val; // in order to maintain total
																				 // accuracy, for now this will
																				 // be all or nothing, but later
																				 // we will change how we do this
			q.execute(query.str());
		}

		for (std::map<std::string, std::vector<std::string> >::iterator itr = G_aisles.begin(); itr != G_aisles.end(); ++itr)
		{
			std::string symbol(itr->first), items;
			std::ostringstream query;

			std::vector<std::string>::iterator jtr = itr->second.begin();
			items += *jtr;
			++jtr;
			for (; jtr != itr->second.end(); ++jtr) {
				items += (", " + *jtr);
			}

			query << "INSERT INTO AisleItemLocator.Aisle (SID, Symbol, Items)";
			query << "VALUES(\'" << sid_val << "\', \'" << symbol << "\', ";
			query << "\'" << items << "\');";
			q.execute(query.str());
		}
	} catch (SimpleDB::Exception & e) {
		std::cerr << "Caught Exception in load_to_aisle_table: " << e.what() << std::endl;
		G_response = "Something went wrong while loading the data to our database for " + G_store_name + " with address " + G_address + ".";
		G_response += " Trying again and/or contact us. Thank you.";
		G_can_insert = false;
		return;
	}

	if (data_exists) {
		G_response = "We have received aisle information for " + G_store_name + " with address " + G_address + "\n\n"; 
		G_response += format_aisles_output(G_aisles) + "and have successfully updated it!";
	} else {
		G_response = "We have received aisle information for " + G_store_name + " with address " + G_address + "\n\n"; 
		G_response += format_aisles_output(G_aisles) + "and have successfully loaded it!";
	}
}

void load_to_store_table(const std::string & street_val
	, const std::string & city_val, const std::string & state_val
	, const std::string & zip_val, const std::string & country_val
	, const int64_t & sid_val)
{
	std::cout << "Begin loading to the store table for" << G_store_name << std::endl;

	std::ostringstream query;
	query << "INSERT INTO AisleItemLocator.Store (Name, Street, City, State, Zip, Country, SID) VALUES(\'";
	query << G_store_name << "\', \'" << street_val << "\', \'" << city_val << "\', \'" << state_val << "\'";
	query << ", \'" << zip_val << "\', \'" << country_val << "\', \'" << sid_val << "\');";

	try {
		SimpleDB::Query q(G_db);
		q.execute(query.str());
	} catch (SimpleDB::Exception & e) {
		std::cerr << "Caught Exception in load_to_store_table: " << e.what() << std::endl;
		G_response = "Something went wrong while loading the data to our database for " + G_store_name + " with address " + G_address + ".";
		G_response += " Trying again and/or contact us. Thank you.";
		G_can_insert = false;
	}
}

void load_to_db() {
	std::string street;
	std::string city;
	std::string state;
	std::string zip;
	std::string country;
	int64_t sid = -1; // this will be generated by the database
					  // or retrieved from it at a later point 

	pre_process_store_keys(street, city, state, zip, country); // initializes all of these
															   // from the data sent in request
	// check if the store exists in our records
	if (check_store_exists(street, city
		, state, zip, country, sid) && G_can_insert) {
		// next we check if we can update this table
		// by querying the status table
		if (is_open_for_update(sid) && G_can_insert) {
			// now we load the data to the aisle table
			load_to_aisle_table(sid, true);
		} else if (G_can_insert) {
			// generate a response that tells the user
			// that the store data cannot be loaded
			G_response = "We cannot load the data for " + G_store_name + " with address " + G_address + " because it is no longer open for update.";
			G_response += " Check back later to see if the status changes or try a different location if this store has multiple locations. Thank you.";
		}
	} else if (G_can_insert) {
		// if it doesn't, load the data to the status table
		// this will generate the sid

		// if something goes wrong in any of the 
		// following steps, then we need to 
		// rollback the entire transaction
		// so we construct this here for that purpose
		SimpleDB::ScopedTransaction transaction(G_db);

		load_to_status_table(sid);

		if (G_can_insert) {
			// we then use this sid to load the aisles to the
			// aisle table
			load_to_aisle_table(sid, false);
		} else {
			return; // rollback the transaction
		}

		if (G_can_insert) {
			// and add it as a foreign key column (in this case simply
			// a non-null value) to the store table as we load it as well
			load_to_store_table(street, city, state, zip, country, sid);
		} else {
			return; // rollback the transaction
		}

		if (!G_can_insert) {
			return; // rollback the transaction
		}

		transaction.commit(); // everything went correctly, commit
	}
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
			if (num_columns == MAX_FILE_COLUMNS) {
				std::cerr << "We have received a file containig >= " << num_columns << " columns which is past ";
				std::cerr << MAX_FILE_COLUMNS << ". We are cutting off here.";
				break; // protect against spamming us with an impossibly
					   // large number of rows for a grocery store
			}
		}

		++num_rows;
		if (num_rows == MAX_FILE_ROWS) {
			std::cerr << "We have received a file containig >= " << num_rows << " rows which is past ";
			std::cerr << MAX_FILE_ROWS << ". We are cutting off here.";
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
			load_to_db();
		} else {
			G_response = "We have failed to receive non-empty aisle info for " + G_store_name + " or we have received it, but we are unable to load it";
			G_response += " because you did not properly specify store name and address (if this is the case you will receive this response as well)";
		}
	} else {
		G_response = "We have received a file for store " + G_store_name + ", however it is not a csv file.";
		G_response += " Please format data as a csv file and resend it. Thank you.";
	}

	// End end of a load transaction
	// we are clearing everything for
	// the next set of requests
	G_aisles.clear();
	G_store_name.clear();
	G_address.clear();
	G_can_insert = false;
}

bool key_data_in_payload(std::string & payload, int start_pos) {
	// store name is already confirmed
	// from our initial search of the request
	// we shouldn't have to search for it here

	start_pos = payload.find("=", start_pos);
	int end_pos = payload.find("&", start_pos);
	int len = end_pos - start_pos;
	G_store_name = payload.substr(start_pos + 1, len - 1);
	G_store_name = url_decode(G_store_name);
	
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
	G_address = url_decode(G_address);
	
	std::cout << G_address << std::endl;

	if (G_address.empty())
		return false;

	return true;
}

void handle_incoming_key_data(std::string & payload, int start_pos) {
	bool is_key_data_in_payload = key_data_in_payload(payload, start_pos);

	if (is_key_data_in_payload) {
		G_response = "We have succeeded in verifying that the store name " + G_store_name + " with address " + G_address + " can be updated!";
		G_can_insert = true;
	} else if (!is_key_data_in_payload) {
		G_response = "Request did not contain either a store name or store address. Please resubmit your request with these fields.";
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