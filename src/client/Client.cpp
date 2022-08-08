#include "Client.h"


int main(int argc, char *argv[])	{
	
	Client client = Client(SERVER_DEFAULT_ADDRESS, SERVER_DEFAULT_PORT);
	client.run();

	return EXIT_SUCCESS;
}

int Client::run() {

	/**
	 * Move networking and UI to separate threads/processes when
	 * either or both become more CPU intensive.
	 * No performance issue to handle all in one thread for now.
	 */

	while(is_running)	{

		// Action all packets types when connected
		if (connection == CONN_STATE_CONNECTED) {
			if(listen(DEFAULT_TIMEOUT)) {
				if (buf[0] != '\0') {
					
					switch (buf[0] - '0') { 
						
						case MSG_STATE_UPDATE:
							std::cout << "[client] state update received from server" << std::endl;
							std::cout << buf;
							break;
						
						case MSG_STATE_UPDATE_SLICE:
							std::cout << "[client] sliced state update received from server" << std::endl;
							std::cout << buf;
							break;
						
						case MSG_AUTH_CHALLENGE:
							std::cout << "[client] auth challenge received" << std::endl;
							break;
						
						case MSG_AUTH_SUCCESS:
							std::cout << "[client] sucessfully authenticated" << std::endl;
							std::cout << buf;
							break;
						
						case MSG_FORCE_TERMINATE:
							std::cout << "[client] forcefully disconnected from server" << std::endl;
							std::cout << buf;
							break;
						
						default:
							std::cout << "[client] unknown message (" << buf[0] << ") from server " << std::endl;
							std::cout << buf;
					}
				}

			// Exit on error or timeout.
			} else {
				std::cout << "[client] connection timed out " << std::endl;
				// is_running = false;
				connection = CONN_STATE_DISCONNECTED;
			}

		// Only action auth and disconnect packets whilst connection pending
		} else if (connection == CONN_STATE_PENDING) {
			if(listen(DEFAULT_TIMEOUT)) {
				if (buf[0] != '\0') {
					switch (buf[0] - '0') { 
						
						case MSG_AUTH_CHALLENGE:
							std::cout << "[client] auth challenge received" << std::endl;
							send_auth_verify_message();
							break;

						case MSG_AUTH_SUCCESS:
							std::cout << "[client] sucessfully authenticated" << std::endl;
							connection = CONN_STATE_CONNECTED;
							break;

						case MSG_FORCE_TERMINATE:
							std::cout << "[client] disconnected from server" << std::endl;
							std::cout << buf;
							connection = CONN_STATE_DISCONNECTED;
							break;
						
						default:
							std::cout << "[client] unknown message (" << buf[0] << ") from server " << std::endl;
							std::cout << buf;
					}
				}
			// Exit on error or timeout.
			} else {
				std::cout << "[client] connection timed out " << std::endl;
				// is_running = false;
				connection = CONN_STATE_DISCONNECTED;
			}


		// Attempt connection if disconnected
		} else if (connection == CONN_STATE_DISCONNECTED) {
			send_auth_init_message();
			connection = CONN_STATE_PENDING;
		}
		
		// Sleep to allow time for server to respond
		std::this_thread::sleep_for(std::chrono::milliseconds(500));

	}

	connection = CONN_STATE_DISCONNECTED;
	close(s);

	return EXIT_SUCCESS;
}

Client::Client(const char* address, int port)  {
	s = sizeof(s_info_server);
	send_len = sizeof(s_info_server);

	// Create socket
	if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)	{
		is_running = false;
	} else {is_running = true;}

	// Store address/port details in addrinfo struct
	memset((char *) &s_info_server, 0, sizeof(s_info_server));
	s_info_server.sin_family = AF_INET;
	s_info_server.sin_port = htons(SERVER_DEFAULT_PORT);
	if (inet_aton(SERVER_DEFAULT_ADDRESS , &s_info_server.sin_addr) == 0) {
		is_running = false;
	} else {is_running = true;}

	// Set the players account details
	set_account_details();

}

Client::Client()    {
    is_running = false;
}

Client::~Client()   {
    close(s);
}

int Client::listen(double timeout) {

	// std::cout << "Time remaining: " << timeout << std::endl;    

	// Set socket timeout
	struct timeval t;
	t.tv_sec = 0;
	t.tv_usec = timeout * 1000000;
	setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char*)&t, sizeof t);

	// Clear buffer and receive message.
	memset(buf,'\0', PACKET_SIZE);
	recv_len = recvfrom(s, buf, PACKET_SIZE, 0, (struct sockaddr *) & s_info_server, (socklen_t*)&send_len);
		
	// Timeout or error
	if (recv_len == -1)	{
		return EXIT_FAILURE;	
	
	// Success
	} else {
	
	}

	return 1;
}

int Client::send_auth_verify_message() {

	std::cout << "[client] responding with auth hash" << std::endl;

	// Extract salt from message.
	char str[SALT_SIZE];
	memcpy(str, &buf[1], SALT_SIZE);
	std::string salt = str;
	str[SALT_SIZE] = '\0';
	salt.erase(std::remove(salt.begin(), salt.end(), ' '), salt.end());
	salt.erase(std::remove(salt.begin(), salt.end(), '\n'), salt.end());
	this->salt = salt;

	// Message format:  [MSG_AUTH]    [USERNAME + PADDING]      [AUTH_ACTION_VERIFY]      [auth hash]     
	// at index:		    0	    1 to USERNAME_MAX_LENGTH   USERNAME_MAX_LENGTH + 1	  next 64 bytes
	memset(msg,'\0', PACKET_SIZE); 
	std::string s_msg;
	s_msg += MSG_AUTH + '0';
	s_msg += username;
	int len = username.length();
	if (len < USERNAME_MAX_LENGTH) {
		s_msg += std::string(USERNAME_MAX_LENGTH - len, ' ');
	}
	s_msg += AUTH_ACTION_VERIFY + '0';
	this->hash = SHA256(password + salt);
	s_msg += this->hash;

	// Copy formatted string to message buffer
	strcpy(msg, s_msg.c_str());
	
	if (sendto(s, msg, strlen(msg) , 0 , (struct sockaddr *) &s_info_server, send_len) == -1)	{
		std::cout << "[client] error sending message" << std::endl;
		return EXIT_FAILURE;
	}
	
	return EXIT_SUCCESS;
}

int Client::send_auth_init_message() {

	std::cout << "[client] sending auth request" << std::endl;
	
	// Message format:  [MSG_AUTH]      [USERNAME + PADDING]         [AUTH_ACTION_INIT]
	// at index:		    0	      1 to USERNAME_MAX_LENGTH	    USERNAME_MAX_LENGTH +1
	memset(msg,'\0', PACKET_SIZE); 
	std::string s_msg;
	s_msg += MSG_AUTH + '0';
	s_msg += username;
	int len = username.length();
	if (len < USERNAME_MAX_LENGTH) {
		s_msg += std::string(USERNAME_MAX_LENGTH - len, ' ');
	}
	s_msg += AUTH_ACTION_INIT + '0';

	// Copy formatted string to message buffer and send
	strcpy(msg, s_msg.c_str());
	if (sendto(s, msg, strlen(msg) , 0 , (struct sockaddr *) &s_info_server, send_len) == -1)	{
		std::cout << "[client] error sending message" << std::endl;
		return EXIT_FAILURE;
	}
	
	return EXIT_SUCCESS;
}

bool Client::running()  {return is_running;}

bool Client::set_account_details()  {
	
	// Get from a file or stdin later
	username = "player1";
	password = "password";

	return true;
}