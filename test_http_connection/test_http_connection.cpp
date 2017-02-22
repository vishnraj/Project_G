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

#define MYPORT "8037"  // the port users will be connecting to
#define BACKLOG 1     // how many pending connections queue will hold
#define MAX_LENGTH 3000 // the length of the receiving buffer

const std::string a_bunch_of_dashes("----------------------------\n");

int main(int argc, char *argv[])
{
	struct sockaddr_storage their_addr;
	socklen_t addr_size;
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

	if (listen(sockfd, BACKLOG) == -1) {
		std::cout << "Failed to listen." << std::endl;
		return 1;
	}

	std::cout << "Listening for connection.\n";

	// now accept an incoming connection:

	addr_size = sizeof their_addr;
	new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size);

	if (new_fd == -1) {
		std::cout << "Failed to accept." << std::endl;
		return 1;
	}

	std::cout << "Accepting. Closing connection to listening socket.\n";

	bool closed = false;
	while (true) {
		char buffer[MAX_LENGTH];
		ssize_t ret = recv(new_fd, buffer, MAX_LENGTH, 0);
		//ssize_t ret = read(sockfd, buffer, length);
		if (ret > 0) {
			std::string request(buffer, ret);
			std::cout << request << '\n';

			const char * response = "Hello, World! - From test_http_connection.\n";
			ssize_t length = strlen(response);
			ret = send(new_fd, response, length, 0);
			if (ret == -1) {
				std::cout << "Failed to send." << std::endl;
				return 1;
			}

			std::cout << "Sent data. Length was " << ret << " bytes. Response length was supposed to be " << length << " bytes." << std::endl;

			std::cout << "Closing current connection.\n" << a_bunch_of_dashes << std::endl;
			close(new_fd);

			closed = true;
		} else if (ret == 0) {
			std::cout << "Client closed connection. Terminating connection.\n" << a_bunch_of_dashes << std::endl;
			close(new_fd);

			closed = true;
		} else {
			std::cout << "We had an invalid request.\n";
		}

		if (closed) {
			/*
			if (bind(sockfd, res->ai_addr, res->ai_addrlen) == -1) {
				int yes = 1;
				if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == -1) {
					std::cout << "Failed to release socket." << std::endl;
					return 1;
				}

				if (bind(sockfd, res->ai_addr, res->ai_addrlen) == -1) {
					std::cout << "Still failed to bind after seemingly succesfully releasing the socket." << std::endl;
				}
			}
		
			std::cout << "Bound.\n";
			*/

			if (listen(sockfd, BACKLOG) == -1) {
				std::cout << "Failed to listen." << std::endl;
				return 1;
			}

			std::cout << "Listening for connection.\n";

			// now accept an incoming connection:

			addr_size = sizeof their_addr;
			new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size);

			if (new_fd == -1) {
				std::cout << "Failed to accept." << std::endl;
				return 1;
			}

			std::cout << "Accepting. Closing connection to listening socket.\n";

			closed = false;
		}
	}

	close(sockfd);
	close(new_fd);
	freeaddrinfo(res); // free the linked list
	return 0;
}