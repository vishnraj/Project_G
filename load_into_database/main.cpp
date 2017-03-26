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
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>

#define MYPORT "8037"  // the port users will be connecting to
#define BACKLOG 1     // how many pending connections queue will hold
#define MAX_LENGTH 3000 // the length of the receiving buffer

const std::string a_bunch_of_dashes("----------------------------\n");

std::vector < std::vector < std::string > > G_aisles;

std::string format_aisles_output() {
	int aisle_num = 1;
	std::ostringstream output;

	for (std::vector<std::vector < std::string > >::iterator i = G_aisles.begin(); i != G_aisles.end(); ++i) {
		output << aisle_num << ": ";
		for (std::vector<std::string>::iterator j = i->begin(); j != i->end(); ++j) {
			output << *j << " ";
		}
		output << "\n\n";
		++aisle_num;
	}

	return output.str();
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

void read_aisle_info(std::string & aisle_info, std::istringstream & buffer) {
	std::string line;
	while (std::getline(buffer, line)) {
		if (std::isspace(line[0]) == 0) {
			aisle_info += line + '\n';
		} else {
			break;
		}
	}
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

void cleanup(int & new_fd, int & sockfd, struct addrinfo * res) {
	close(sockfd);
	close(new_fd);
	freeaddrinfo(res); // free the linked list
}

void parse_csv(const std::string & aisle_info) {
	std::istringstream iss(aisle_info);
	std::string line;
	int i = 0;
	while (getline(iss, line)) {
		std::string::iterator end_pos = std::remove(line.begin(), line.end(), ' ');
		line.erase(end_pos, line.end());

		G_aisles.push_back(std::vector<std::string>());

		std::size_t start_pos = 0;
		std::size_t pos = line.find(",");
		std::size_t len = pos - start_pos;
		bool more_columns = true;
		while (more_columns) {
			if (line[start_pos] != '\0' && line[start_pos] != ',') {
				std::string item = line.substr(start_pos, len);

				G_aisles[i].push_back(item);
				
				start_pos = pos + 1;
				pos = line.find(",", start_pos);
				len = pos - start_pos;
				more_columns = (pos == std::string::npos ? false : true);
			} else {
				more_columns = false;
			}
		}

		++i;
	}
}

void load_to_db(const std::string & aisle_info, const std::string & store_name) {
	parse_csv(aisle_info);

	// takes the current aisle_info vector and loads it into the database
}

std::string get_store_name(std::string filename) {
	int pos(0);
	pos = filename.find("=");
	std::string store_name = filename.substr(pos, std::string::npos);
	pos = store_name.find("\"");
	store_name.erase(0, pos + 1);
	pos = store_name.find("\"");
	store_name.erase(pos);
	pos = store_name.find(".csv");
	store_name.erase(pos);

	return store_name;
}

void handle_request(ssize_t & ret, char * buffer, bool closed, int & new_fd, int & sockfd) {
	if (ret > 0) {
		std::string request(buffer, ret);
		std::istringstream iss(request);
		std::string line;
		std::string store_name;
		std::string aisle_info;

		while (std::getline(iss, line)) {
			int pos(0);
			if ((pos = line.find("filename")) != std::string::npos) {
				std::string filename = line.substr(pos, std::string::npos);
				store_name = get_store_name(filename);
				
				std::getline(iss, line); std::getline(iss, line);
				read_aisle_info(aisle_info, iss);
				break;
			}
		}

		load_to_db(aisle_info, store_name);
		std::ostringstream greeting;
		greeting << "<html>\r\n<body>\r\nLoading aisle data for:\n" << store_name << ":\n\n" << format_aisles_output() << "\r\n</body>\r\n</html>\r\n";
		std::ostringstream response;
		response << "HTTP/1.0 200 OK\r\nContent-Length: " << greeting.str().length() << "\r\nContent-Type: text/html\r\n\r\n" << greeting.str();
		ret = send(new_fd, response.str().c_str(), response.str().length(), 0);
		if (ret == -1) {
			std::cout << "Failed to send." << std::endl;
			exit(EXIT_FAILURE);
		}

		std::cout << "Sent data. Length was " << ret << " bytes. Response length was supposed to be " << response.str().length() << " bytes.\n";
		std::cout << "Response:\n" << greeting.str() << std::endl;
	} else if (ret == 0) {
		std::cout << "Client closed connection. Terminating connection.\n" << a_bunch_of_dashes << std::endl;
		close(new_fd);

		closed = true;
	} else {
		std::cout << "We had an invalid request.\n";
	}

	if (closed) {
		new_fd = instantiate_connection(sockfd);
		if (new_fd == -1) {
			exit(EXIT_FAILURE);
		}
		closed = false;
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
		handle_request(ret, buffer, closed, new_fd, sockfd);
	}

	cleanup(new_fd, sockfd, res);
	return 0;
}