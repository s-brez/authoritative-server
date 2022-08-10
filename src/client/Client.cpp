#include "Client.h"


int main(int argc, char *argv[])	{
	
	Client client = Client(SERVER_DEFAULT_ADDRESS, SERVER_DEFAULT_PORT);
	client.run();

	return EXIT_SUCCESS;
}

Client::Client(const char* address, int port)  {

	// Init socket
	s = sizeof(s_info_server);
	send_len = sizeof(s_info_server);
	if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)	{
		should_run = false;
	} else {should_run = true;}

	// Set socket timeout
	struct timeval t;
	t.tv_sec = 0;
	t.tv_usec = 10;
	setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char*)&t, sizeof t);

	// Store address/port details in addrinfo struct
	memset((char *) &s_info_server, 0, sizeof(s_info_server));
	s_info_server.sin_family = AF_INET;
	s_info_server.sin_port = htons(SERVER_DEFAULT_PORT);
	if (inet_aton(SERVER_DEFAULT_ADDRESS , &s_info_server.sin_addr) == 0) {
		should_run = false;
	} else {should_run = true;}

	// Set the players account details
	set_account_details();

	connection = CONN_STATE_DISCONNECTED;

}

Client::Client()    {
    should_run = false;
}

Client::~Client()   {
    close(s);
}

int Client::listen() {

	// std::cout << "Time remaining: " << timeout << std::endl;    

	// Clear buffer and receive message.
	memset(buf,'\0', PACKET_SIZE);
	recv_len = recvfrom(s, buf, PACKET_SIZE, 0, (struct sockaddr *) & s_info_server, (socklen_t*)&send_len);
		
	// Timeout or error
	if (recv_len == -1)	{
		return false;	
	
	// Success
	} else {
		return true;
	}

	return 1;
}

int Client::run() {

	double ms_per_tick = 1000 / TICKS_PER_SECOND_CLIENT;

	std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
	std::chrono::system_clock::time_point finish = std::chrono::system_clock::now();
	std::chrono::duration<double, std::milli> processing_time;

	while(should_run)	{

		start = std::chrono::system_clock::now();
		processing_time = start - finish;
		
		// std::cout << "processing time: " << processing_time.count() << std::endl;

		if(processing_time.count() < ms_per_tick) {
			std::chrono::duration<double, std::milli> delta(ms_per_tick - processing_time.count());
			auto delta_duration = std::chrono::duration_cast<std::chrono::milliseconds>(delta);
			// std::cout << "sleep for " << delta_duration.count() << std::endl;
			std::this_thread::sleep_for(std::chrono::milliseconds(delta_duration.count()));
		}

		finish = std::chrono::system_clock::now();
        std::chrono::duration<double, std::milli> sleep_time = finish - start;
            
        while(listen()) {
			if (connection == CONN_STATE_CONNECTED) {				  
				if (buf[0] != '\0') {
					switch (buf[0] - '0') { 
						
						case MSG_STATE_UPDATE:
							// std::cout << "[client] state update received from server" << std::endl;
							process_state_update_packet();
							break;
						
						case MSG_STATE_UPDATE_SLICE:
							std::cout << "[client] sliced state update received from server" << std::endl;
							std::cout << buf;
							break;
						
						case MSG_AUTH_CHALLENGE:
							std::cout << "[client] auth challenge received" << std::endl;
							send_auth_verify_packet();
							break;
						
						case MSG_AUTH_SUCCESS:
							std::cout << "[client] sucessfully authenticated" << std::endl;
							std::cout << buf;
							break;
						
						case MSG_FORCE_TERMINATE:
							std::cout << "[client] disconnected from server" << std::endl;
							std::cout << buf;
							connection = CONN_STATE_DISCONNECTED;
							should_run = false;
							break;
						
						default:
							std::cout << "[client] unknown message (" << buf[0] << ") from server " << std::endl;
							std::cout << buf;
					}
				}
			
				send_input();

			// Only action auth and disconnect packets whilst connection pending
			} else if (connection == CONN_STATE_PENDING) {
				if (buf[0] != '\0') {
					switch (buf[0] - '0') { 
						
						case MSG_AUTH_CHALLENGE:
							std::cout << "[client] auth challenge received" << std::endl;
							send_auth_verify_packet();
							break;

						case MSG_AUTH_SUCCESS:
							std::cout << "[client] sucessfully authenticated" << std::endl;
							connection = CONN_STATE_CONNECTED;
							break;

						case MSG_FORCE_TERMINATE:
							std::cout << "[client] disconnected from server" << std::endl;
							std::cout << buf;
							connection = CONN_STATE_DISCONNECTED;
							should_run = false;
							break;
						
						default:
							std::cout << "[client] unknown message (" << buf[0] << ") from server " << std::endl;
							std::cout << buf;
					}
				}
			}
		}

		// Attempt connection if disconnected
		if (connection == CONN_STATE_DISCONNECTED) {
			send_auth_init_packet();
			connection = CONN_STATE_PENDING;
		}

		// std::cout << "Time: " << processing_time.count() + sleep_time.count() << std::endl;
	}

	connection = CONN_STATE_DISCONNECTED;
	close(s);

	return EXIT_SUCCESS;
}

int Client::process_state_update_packet() {
	
	// Check username in packet matches client username.
	if (strstr(buf, username.c_str())) {
		
		// Copy data from header
		char data[FREE_SPACE_PER_PACKET] ;
		memcpy(data, &buf[ID_HEADER_SIZE], FREE_SPACE_PER_PACKET);

		// --------------------------
		// ** Deserialise and action state data here **


		// ** Deserialise and action state data here **
		// --------------------------
	}

	return EXIT_SUCCESS;
}

int Client::send_input() {
	
	std::cout << "[client] sending input to:" << std::endl;
	// std::cout << inet_ntoa(s_info_server.sin_addr) << " " << s_info_server.sin_port <<std::endl;

	// --------------------------
	// ** Serialise input here **

	std::string serialised_input = "This is test serialised input data.";
	
	// ** Serialise input here **
	// --------------------------

	// Message format:  [MSG_INPUT]    [USERNAME + PADDING]        [spare byte]                    [input data]     
	// at index:		    0	    1 to USERNAME_MAX_LENGTH   USERNAME_MAX_LENGTH + 1	  USERNAME_MAX_LENGTH to end of buffer
	memset(imsg,'\0', INPUT_PACKET_SIZE); 
	std::string i_msg;
	i_msg += MSG_INPUT + '0';
	i_msg += username;
	int len = username.length();
	if (len < USERNAME_MAX_LENGTH) {
		i_msg += std::string(USERNAME_MAX_LENGTH - len, ' ') + ' ';
	}
	
	i_msg += serialised_input;

	// Copy formatted string to message buffer
	strcpy(imsg, i_msg.c_str());
	
	if (sendto(s, imsg, strlen(imsg) , 0 , (struct sockaddr *) &s_info_server, send_len) == -1)	{
		std::cout << "[client] error sending input" << std::endl;
		memset(imsg,'\0', INPUT_PACKET_SIZE); 
		return EXIT_FAILURE;
	}

	memset(imsg,'\0', INPUT_PACKET_SIZE); 
	return EXIT_SUCCESS;
}

int Client::send_auth_verify_packet() {

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
		memset(msg,'\0', PACKET_SIZE); 
		return EXIT_FAILURE;
	}
	
	memset(msg,'\0', PACKET_SIZE); 
	return EXIT_SUCCESS;
}

int Client::send_auth_init_packet() {

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

bool Client::running()  {return should_run;}

bool Client::set_account_details()  {
	
	// Get from a file or stdin later
	username = "player1";
	password = "password";

	return true;
}