#include "Client.h"


int main(int argc, char *argv[])	{
	
	Client client = Client(SERVER_DEFAULT_ADDRESS, SERVER_DEFAULT_PORT);

	char buf[PACKET_SIZE];
	char msg[PACKET_SIZE];

	while(client.running())	{

		// Send message to server
		client.send(buf, msg, PACKET_SIZE);

		// Print response, if any.
		if (buf[0] != '\0') {std::cout << buf;} 

	}

	close(client.s);

	return EXIT_SUCCESS;
}

Client::Client(const char* address, int port)  {
	s = sizeof(s_info_server);
	send_len = sizeof(s_info_server);

	// Create socket
	if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)	{
		is_running = false;
	} else {is_running = true;}

	// Store address/port details in addrinfo struct
	memset((char *) &s_info_server, 0, sizeof(s_info_server));
	s_info_server.sin_family = AF_INET;
	s_info_server.sin_port = htons(SERVER_DEFAULT_PORT);
	if (inet_aton(SERVER_DEFAULT_ADDRESS , &s_info_server.sin_addr) == 0) {
		is_running = false;
	} else {is_running = true;}
}

Client::Client()    {
    is_running = false;
}

Client::~Client()   {
    close(s);
}

int Client::send(char *buf, char *msg, int max_size) {

	printf("Enter message : ");
	fgets(msg, max_size, stdin);
	
	// Send message.
	if (sendto(s, msg, strlen(msg) , 0 , (struct sockaddr *) &s_info_server, send_len)==-1)	{
		std::cout << "[client] error sending message" << std::endl;
	}
	
	// Clear buffer and receive response.
	memset(buf,'\0', PACKET_SIZE);
	if (recvfrom(s, buf, PACKET_SIZE, 0, (struct sockaddr *) &s_info_server, (socklen_t*)&send_len) == -1)	{
		std::cout << "[client] error reading message response packet" << std::endl;
	}	

	return 1;
}

bool Client::running()  {return is_running;}