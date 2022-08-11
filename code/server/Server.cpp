#include "Server.h"


int main(int argc, char *argv[])	{
	
    Server server = Server(SERVER_DEFAULT_ADDRESS, SERVER_DEFAULT_PORT);
	server.run();

	return EXIT_SUCCESS;
}

Server::Server(const char* address, int port) {

	// Init socket
	s = sizeof(s_info_client);
	send_len = sizeof(s_info_client);
	if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)	{
		should_run = false;
		std::cout << "[server] failed to create socket" << std::endl;
	} else {should_run = true;}
	
	// Set socket timeout.
	struct timeval t;
	t.tv_sec = 0;
	t.tv_usec = 10;
	setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char*)&t, sizeof t);

	// Store address/port details
	memset((char *) &s_info_server, 0, sizeof(s_info_server));
	s_info_server.sin_family = AF_INET;
	s_info_server.sin_port = htons(port);
	s_info_server.sin_addr.s_addr = inet_addr(address); 	// Specified address
	// s_info.sin_addr.s_addr = htonl(INADDR_ANY);			// Localhost

	// Bind socket to specified port.
	if(bind(s, (struct sockaddr*)&s_info_server, sizeof(s_info_server) ) == -1){
		should_run = false;
		std::cout << "[server] failed to bind socket" << std::endl;
	} else {should_run = true;}

	// Fetch known account details for login auth.
	if (update_known_accounts()) {
		std::cout << "[server] failed to fetch user accounts" << std::endl;
	} else {should_run = true;}

	// Set client connection slots to empty
	total_connected_clients = 0;
	for (int i = 0; i < MAX_PLAYERS; i++ ) {
		ClientState state;
		state.in_use = false;
		slots[i] = state;
	}
	
}
    
Server::~Server() {
	close(s);
}

Server::Server() {}

int Server::listen() {

	// Clear buffer and receive message.
	memset(buf,'\0', PACKET_SIZE);
	recv_len = recvfrom(s, buf, PACKET_SIZE, 0, (struct sockaddr *) &s_info_client, (socklen_t*)&send_len);
	
	// Timeout or error
	if (recv_len == -1)	{
		return false;	
	
	// Success
	} else {
		return true;
	}

}

int Server::run() {
	
	double ms_per_tick = 1000 / TICKS_PER_SECOND_SERVER;

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

		/** Packet validation rules: 
		 * 1. Three possible client packet types: input, chat, auth.
		 * 2. First byte of all packet data must be MSG_<TYPE> integer.
		 * 3. Packets other than auth handshake init must come from a logged-in IP.
		 */
		while(listen()) {
			if (buf[0] != '\0') {
				switch (buf[0] - '0') {  // Convert char byte to int for type checking
					
					case MSG_INPUT:
						// std::cout << "[server] input received from client " << inet_ntoa(s_info_client.sin_addr) << std::endl;
						process_input_packet();
						break;

					case MSG_CHAT:
						// std::cout << "[server] chat received from client " << inet_ntoa(s_info_client.sin_addr) << std::endl;
						process_chat_packet();
						break;

					case MSG_AUTH:
						process_auth_packet();
						break;
				
					default:
						std::cout << "[server] unknown message (" << buf[0] << ") from client " << std::endl;
				}
			}
		}

		// Send state update packets to clients and decrement last heard.
		if (total_connected_clients > 0) {
			std::cout << "[server] broadcasting state update" << std::endl;
			for (int i = 0; i < MAX_PLAYERS; i++) {
				if (slots[i].in_use) {
					// std::cout << inet_ntoa(slots[i].addr.sin_addr) << " " << slots[i].addr.sin_port <<std::endl;
					slots[i].last_heard  -= ms_per_tick;
					send_state_update(slots[i]);
				}
			}
		}



		// std::cout << "Time: " << processing_time.count() + sleep_time.count() << std::endl;
	}

	close(s);
   return EXIT_SUCCESS;	

}

int Server::send_state_update(ClientState client) {

	// Single packet messages:
	// Message format:  [MSG_STATE_UPDATE]      [update data]
	// at index:		        0	          1 to end of buffer

	// For infrequent large updates use multiple slice protocol.

	// --------------------------
	// ** Serialise state here **

	std::string state_data = "This is test state update data.";
	
	// ** Serialise state here **
	// --------------------------

	// Clear message buffer
	memset(msg,'\0', PACKET_SIZE);

	// Format packet
	std::string payload = "";
	payload += MSG_STATE_UPDATE + '0';
	payload += client.username;
	int len = client.username.length();
	if (len < USERNAME_MAX_LENGTH) {
		payload += std::string(USERNAME_MAX_LENGTH - len, ' ');
	}
	payload += " " + state_data; 
	strcpy(msg, payload.c_str());

	if (sendto(s, msg, strlen(msg), 0, (struct sockaddr*) &client.addr, send_len) == -1)	{
		std::cout << "[server] error when sending state packet" << std::endl;
		memset(msg,'\0', PACKET_SIZE);
		return EXIT_FAILURE;
	}	

	memset(msg,'\0', PACKET_SIZE);
	return 1;
}

int Server::process_input_packet() {

	// Extract username from message buffer.
	char str[USERNAME_MAX_LENGTH];
	memcpy(str, &buf[1], USERNAME_MAX_LENGTH);
	std::string padded_username = str;
	std::string username;	
	str[USERNAME_MAX_LENGTH] = '\0';

	// Stop when ' ' or USERNAME_MAX_LENGTH is reached.
	bool done = false;
	for (int i = 0; i < USERNAME_MAX_LENGTH && !done; i ++) {
		if (padded_username[i] != ' ') {
			username += padded_username[i];
		} else {
			done = true;
		}
	}
	
	// Sender IP must match stored IP and username.
	done = false;
	bool valid = false;
	for (int i = 0; i < MAX_PLAYERS && !done; i++ ) {
        if (slots[i].in_use) {
			if (slots[i].username == username) {
				slots[i].last_heard = LAST_HEARD_TIMEOUT;
				done = true;
				if (inet_ntoa(slots[i].addr.sin_addr) == inet_ntoa(s_info_client.sin_addr)) {
					valid = true;
				}
			}
		}
	}

	if (valid) {
		// Do something with input here.
	}

	return 1;
}

int Server::process_chat_packet() {
	return 1;
}

int Server::process_auth_packet() {

	/** 
	 * Auth handshake & message validation: 
	 * 
	 * - Client message buf[1] to buf[USERNAME_MAX_LENGTH + 1] must contain client username.
	 * - Send deny message if no known username matches.
	 * 
	 * - Client message buf[USERNAME_MAX_LENGTH + 2] must be int 1 or 2, with cases:
	 * 		(INIT) 1: Send client challenge packet with salt, wait for response.
	 * 		(AUTH) 2: 64 chars after buf[USERNAME_MAX_LENGTH + 5] must be SHA-256 auth hash
	 * - Send deny message if invalid message format.
	 * 
	 * - Client message buf[USERNAME_MAX_LENGTH + 3] must be int 1 or 2, with cases:
	 * 		(LOGIN)  1: If hash and username matches, log the client in.
	 * 		(LOGOUT) 2: If hash and username matches, and client logged in, log client out.
	 * - Send deny message and cancel login handshake if invalid message format or hash mismatch.
	 */

	// Extract username from message buffer.
	char str[USERNAME_MAX_LENGTH];
	memcpy(str, &buf[1], USERNAME_MAX_LENGTH);
	std::string padded_username = str;
	std::string username;	
	str[USERNAME_MAX_LENGTH] = '\0';

	// Stop when ' ' or USERNAME_MAX_LENGTH is reached.
	bool done = false;
	for (int i = 0; i < USERNAME_MAX_LENGTH && !done; i ++) {
		if (padded_username[i] != ' ') {
			username += padded_username[i];
		} else {
			done = true;
		}
	}

	// Check client connection state.
	int slot = -1;
	if (std::find(usernames.begin(), usernames.end(), username) != usernames.end()) {

		// Allocate client to a connection slot if not already allocated.
		slot = allocate_client_connection_slot(username, s_info_client);
		if (slot >= 0) {

			// Check buf[USERNAME_MAX_LENGTH + 1] for intended action.
			switch (buf[USERNAME_MAX_LENGTH + 1] - '0') {
				
				// Client sent auth init request, respond with challenge packet.
				case AUTH_ACTION_INIT:
					std::cout << "[server] auth init message received from " << username  << std::endl;	
					slots[slot].connected = CONN_STATE_PENDING;
					send_challenge_message(slots[slot]);
					break;

				// Check incoming hash against actual hash. 
				case AUTH_ACTION_VERIFY:
					std::cout << "[server] checking " << username << " credentials" << std::endl;	
					if(verify_auth_hash(slots[slot].login_hash, buf)) {
						std::cout << "[server] " << username << " authenticated" << std::endl;	
						slots[slot].connected = CONN_STATE_CONNECTED;
						slots[slot].last_heard = LAST_HEARD_TIMEOUT;
						total_connected_clients++;
						send_success_message(slots[slot]);
					} else {
						std::cout << "[server] " << username << " failed to authenticate" << std::endl;	
						send_terminate_message(slots[slot]);
						ClientState empty_slot;
						empty_slot.in_use = false;
						slots[slot] = empty_slot;
					}
					
					break;
				
				// Client send disconnect request, terminate connection.
				case AUTH_ACTION_DISCONNECT:
					std::cout << "[server] " << username << " requested disconnection" << std::endl;	
					send_terminate_message(slots[slot]);
					ClientState empty_slot;
					empty_slot.in_use = false;
					slots[slot] = empty_slot;
					total_connected_clients--;
					break;
			}
		
		// Deny access on error case
		} else {
			send_terminate_message(slots[slot]);
		}
		
	// Deny if username not known
	} else {
		std::cout << "[server] username " << username << " does not exist." << std::endl;
		send_terminate_message(slots[slot]);	
	}

	return EXIT_SUCCESS;
}

bool Server::verify_auth_hash(std::string actual, char* test) {

	char str[SHA256_DIGEST_LENGTH*2];
	memcpy(str, &test[USERNAME_MAX_LENGTH + 2], SHA256_DIGEST_LENGTH*2);
	str[SHA256_DIGEST_LENGTH*2] = '\0';
	std::string incoming = str;

	return actual == incoming;
}

int Server::send_success_message(ClientState client) {
	std::cout << "[server] sending success message to " << client.username << std::endl;	
	
	// Clear message buffer
	memset(msg,'\0', PACKET_SIZE);
	
	// First char is message type
	// Next 32 chars is the clients salt
	std::string s_msg;
	s_msg += MSG_AUTH_SUCCESS + '0';
	s_msg += client.login_salt;
	
	// Copy formatted string to message buffer
	strcpy(msg, s_msg.c_str());

	// std::cout  	<< "[server] sending auth success message to: \n"
	// 			<< "         " << client.username << " at " << inet_ntoa(client.addr.sin_addr) << ":" 
	// 			<< client.addr.sin_port << " (slot " << client.index << ")\n"
	// 			<< "         salt: " << client.login_salt << '\n'
	// 			<< "         hash: " << client.login_hash << '\n'
	// 			<< "         msg: " << msg << std::endl;

	if (sendto(s, msg, strlen(msg), 0, (struct sockaddr*) &client.addr, send_len) == -1)	{
		std::cout << "[server] error when sending response packet" << std::endl;
		memset(msg,'\0', PACKET_SIZE);
		return EXIT_FAILURE;
	}	
	
	memset(msg,'\0', PACKET_SIZE);
	return 1;
}

int Server::send_terminate_message(ClientState client) {

	std::cout << "[server] sending termination message to " << client.username << std::endl;	

	memset(msg,'\0', PACKET_SIZE);
	
	std::string s_msg;
	s_msg += MSG_FORCE_TERMINATE + '0';
	s_msg += client.login_salt;
	
	strcpy(msg, s_msg.c_str());

	if (sendto(s, msg, strlen(msg), 0, (struct sockaddr*) &client.addr, send_len) == -1)	{
		std::cout << "[server] error when sending response packet" << std::endl;
		return EXIT_FAILURE;
	}	

	return 1;
}

int Server::send_challenge_message(ClientState client) {

	memset(msg,'\0', PACKET_SIZE);
	
	std::string s_msg;
	s_msg += MSG_AUTH_CHALLENGE + '0';
	s_msg += client.login_salt;
	
	strcpy(msg, s_msg.c_str());

	if (sendto(s, msg, strlen(msg), 0, (struct sockaddr*) &client.addr, send_len) == -1)	{
		std::cout << "[server] error when sending response packet" << std::endl;
		return EXIT_FAILURE;
	}	

	return 1;

}

int Server::allocate_client_connection_slot(std::string username, sockaddr_in ip) {

	int first_free_slot = -1;
	bool done = false;
	std::string temp_pass = "";

	// Check all slots to see if player already connected
	for (int i = 0; i < MAX_PLAYERS && !done; i++ ) {
        if (slots[i].in_use) {
			
			// Client already allocated.
			if (slots[i].username == username ) {
				done = true;
				return i;
			}
		
		// Save first free slot in case player not allocated
		} else if (first_free_slot < 0) {
			first_free_slot = i;
		}	
	}	
    
	// Allocate client to first free slot.
	if (!done && first_free_slot >= 0) {
		
		ClientState client;
		
		// IP and port info
		client.addr.sin_family = ip.sin_family;	
		client.addr.sin_port = ip.sin_port;
		client.addr.sin_addr.s_addr = ip.sin_addr.s_addr;

		client.in_use = true;
		client.connected = CONN_STATE_PENDING;
		client.index = first_free_slot;
		client.login_salt = salt();
		client.login_hash = SHA256(get_user_details(username).password + client.login_salt);
		client.username = username;
		
		slots[first_free_slot] = client;
		
		return first_free_slot;
	} 

	// Error case if no free slots and client not allocated.
	return EXIT_FAILURE;
}

AccountInfo Server::get_user_details(std::string username) {
	
	AccountInfo info;
	bool done = false;
	
	for (long unsigned int i = 0; i < accounts.size() && !done; i++ ) {
		if (accounts[i].username == username) {
			info.username = accounts[i].username;
			info.password = accounts[i].password;
		}
	}

	return info;
}

int Server::update_known_accounts() {
	
	// Get this from DB later
	AccountInfo p1 {"player1", "password"};
	AccountInfo p2 {"player2", "password"};
	AccountInfo p3 {"sixteencharname_", "password"};
	AccountInfo p4 {"nameovercharacterlimit", "password"};
	accounts.push_back(p1);
	accounts.push_back(p2);
	accounts.push_back(p3);
	accounts.push_back(p4);

	usernames.push_back("player1");
	usernames.push_back("player2");
	usernames.push_back("sixteencharname_");
	usernames.push_back("nameovercharacterlimit");

	return EXIT_SUCCESS;

}

bool Server::running()  {return should_run;}