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

#define MYPORT "8037"  // the port users will be connecting to
#define BACKLOG 1     // how many pending connections queue will hold
#define MAX_LENGTH 3000 // the length of the receiving buffer

const std::string a_bunch_of_dashes("----------------------------\n");

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

int main(int argc, char *argv[])
{
	struct addrinfo hints, *res;
	int sockfd, new_fd;

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
		return 1;
	}

	std::cout << "Bound.\n";

	new_fd = instantiate_connection(sockfd);
	if (new_fd == -1) {
		return 1;
	}

	bool closed = false;
	while (true) {
		char buffer[MAX_LENGTH];
		ssize_t ret = recv(new_fd, buffer, MAX_LENGTH, 0);
		//ssize_t ret = read(sockfd, buffer, length);
		if (ret > 0) {
			std::string request(buffer, ret);
			std::cout << request << '\n';

			std::string greeting("<html>\r\n<body>\r\nHello, World! - From test_http_connection.\r\n</body>\r\n</html>\r\n");
			std::ostringstream response;
			response << "HTTP/1.0 200 OK\r\nContent-Length: " << greeting.length() << "\r\nContent-Type: text/html\r\n\r\n" << greeting;
			ret = send(new_fd, response.str().c_str(), response.str().length(), 0);
			if (ret == -1) {
				std::cout << "Failed to send." << std::endl;
				return 1;
			}

			std::cout << "Sent data. Length was " << ret << " bytes. Response length was supposed to be " << response.str().length() << " bytes.\n";
			std::cout << "Response:\n" << greeting << std::endl;
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
				return 1;
			}
			closed = false;
		}
	}

	close(sockfd);
	close(new_fd);
	freeaddrinfo(res); // free the linked list
	return 0;
}