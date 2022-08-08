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
		is_running = false;
		std::cout << "[server] failed to create socket" << std::endl;
	} else {is_running = true;}
	
	// Store address/port details
	memset((char *) &s_info_server, 0, sizeof(s_info_server));
	s_info_server.sin_family = AF_INET;
	s_info_server.sin_port = htons(port);
	s_info_server.sin_addr.s_addr = inet_addr(address); 	// Specified address
	// s_info.sin_addr.s_addr = htonl(INADDR_ANY);			// Localhost

	// Bind socket to specified port.
	if(bind(s, (struct sockaddr*)&s_info_server, sizeof(s_info_server) ) == -1){
		is_running = false;
		std::cout << "[server] failed to bind socket" << std::endl;
	} else {is_running = true;}

	// Fetch known account details for login auth.
	if (update_known_accounts()) {
		std::cout << "[server] failed to fetch user accounts" << std::endl;
	} else {is_running = true;}

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

int Server::listen(double timeout) {

	// std::cout << "Time remaining: " << timeout << std::endl;    

	// Set socket timeout
	struct timeval t;
	t.tv_sec = 0;
	t.tv_usec = timeout * 1000000;
	setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char*)&t, sizeof t);

	// Clear buffer and receive message.
	memset(buf,'\0', PACKET_SIZE);
	recv_len = recvfrom(s, buf, PACKET_SIZE, 0, (struct sockaddr *) &s_info_client,  (socklen_t*)&send_len);
	
	// Timeout or error
	if (recv_len == -1)	{
		return EXIT_FAILURE;	
	
	// Success
	} else {
	
	}

	return EXIT_SUCCESS;
}

int Server::run() {
	
	double seconds_per_tick = 1.0f / TICKS_PER_SECOND_SERVER;
	double time_remaining;
	std::chrono::system_clock::time_point start; 
	std::chrono::system_clock::time_point finish;
	std::chrono::duration<double, std::milli> time_elapsed;
	while(is_running)	{
		
		// State updates sent at (1 / TICKS_PER_SECOND_SERVER) intervals.
        finish = std::chrono::system_clock::now();
		start = std::chrono::system_clock::now();
        time_elapsed = start - finish;
		while (time_elapsed.count() < seconds_per_tick) {
            
            start = std::chrono::system_clock::now();
            time_elapsed = start - finish;
			time_remaining = seconds_per_tick - time_elapsed.count();
			
			std::cout << "Listening for: " << time_remaining << std::endl;   

			/** Packet validation rules: 
			 * 1. Three possible client packet types: input, chat, auth.
			 * 2. First byte of all packet data must be MSG_<TYPE> integer.
			 * 3. Packets other than auth handshake init must come from a logged-in IP.
			 */
			listen(time_remaining); 
			if (buf[0] != '\0') {
				switch (buf[0] - '0') {  // Convert char byte to int for type checking
					
					case MSG_INPUT:
						std::cout << "[server] input received from client " << inet_ntoa(s_info_client.sin_addr) << std::endl;
						handle_input_messages();
						break;

					case MSG_CHAT:
						std::cout << "[server] chat received from client " << inet_ntoa(s_info_client.sin_addr) << std::endl;
						handle_chat_messages();
						break;

					case MSG_AUTH:
						handle_auth_messages();
						break;
				
					default:
						std::cout << "[server] unknown message (" << buf[0] << ") from client " << std::endl;
				}
			}

	        std::cout << "Processing time:" << time_elapsed.count() << std::endl;   
		
			finish = std::chrono::system_clock::now(); 

		}
				
        // Send state update packets to each client.
		if (total_connected_clients > 0) {
			std::cout << "[server] sending state update to clients" << std::endl;
		}
	}

	close(s);
   return EXIT_SUCCESS;	

}

int Server::handle_input_messages() {
	return 1;
}

int Server::handle_chat_messages() {
	return 1;
}

int Server::handle_auth_messages() {

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
	std::string username = str;
	str[USERNAME_MAX_LENGTH] = '\0';
	username.erase(std::remove(username.begin(), username.end(), ' '), username.end());
	username.erase(std::remove(username.begin(), username.end(), '\n'), username.end());
	
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
		return EXIT_FAILURE;
	}	

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


	// Debug: print clients
	// std::cout << "[server] logged in or pending clients:" << std::endl;
	// for (int i = 0; i < MAX_PLAYERS; i++ ) {
	// 	if (slots[i].in_use) {
	// 		std::cout << "    " << slots[i].username << std::endl;
	// 	}	
	// }


	// Return error if no free slots and client not allocated.
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

bool Server::running()  {return is_running;}