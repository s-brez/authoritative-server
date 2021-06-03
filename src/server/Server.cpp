#include "Server.h"

#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<arpa/inet.h>
#include<sys/socket.h>

int main(int argc, char *argv[])	{
	
    Server server = Server(SERVER_DEFAULT_ADDRESS, SERVER_DEFAULT_PORT);
	
	// Get and action packets from clients
	char buf[MSG_BUFFER_MAX_SIZE];
	while(server.running())	{
		
		server.listen(buf, MSG_BUFFER_MAX_SIZE);
		if (buf[0] != '\0') {
			std::cout << buf;
		} 
	}

	close(server.s);

	return EXIT_SUCCESS;
}

Server::Server(const char* address, int port) {

	// Init socket
	s = sizeof(s_info_client);
	send_len = sizeof(s_info_client);
	if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)	{
		is_running = false;
		std::cout << "[server] failed to create socket" << std::endl;
	} else {is_running = true;}
	
	// Store address/port details in addrinfo struct
	memset((char *) &s_info_server, 0, sizeof(s_info_server));
	s_info_server.sin_family = AF_INET;
	s_info_server.sin_port = htons(port);
	s_info_server.sin_addr.s_addr = inet_addr(address);
	// s_info.sin_addr.s_addr = htonl(INADDR_ANY);

	// Bind socket to specified port
	if( bind(s, (struct sockaddr*)&s_info_server, sizeof(s_info_server) ) == -1){
		is_running = false;
		std::cout << "[server] failed to bind socket" << std::endl;
	} else {is_running = true;}

}
    
Server::~Server() {
	close(s);
}

Server::Server() {is_running = false;}

int Server::listen(char* buf, int max_size) {
	
	// Clear buffer and receive message.
	memset(buf,'\0', MSG_BUFFER_MAX_SIZE);
	recv_len = 0;
	if ((recv_len = recvfrom(s, buf, MSG_BUFFER_MAX_SIZE, 0, (struct sockaddr *) &s_info_client,  (socklen_t*)&send_len)) == -1)	{
		std::cout << "[server] error when reading incoming packet" << std::endl;
	}

	// Send response message.
	if (sendto(s, buf, recv_len, 0, (struct sockaddr*) &s_info_client, send_len) == -1)	{
		std::cout << "[server] error when sending response packet" << std::endl;
	}	

	return 1;
} 

bool Server::running()  {return is_running;}