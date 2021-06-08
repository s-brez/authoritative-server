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
	for (int i = 0; i < MAX_PLAYERS; i++ ) {
		ClientState state;
		state.in_use = false;
		slots[i] = state;
	}
}
    
Server::~Server() {
	close(s);
}

Server::Server() {is_running = false;}

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
	
		// Send response message.
		if (sendto(s, buf, recv_len, 0, (struct sockaddr*) &s_info_client, send_len) == -1)	{
			std::cout << "[server] error when sending response packet" << std::endl;
		}	
	
	}

	return EXIT_SUCCESS;
}

int Server::update_known_accounts() {
	
	// Get this from DB later
	AccountInfo p1 {"player1", "password"};
	AccountInfo p2 {"player2", "password"};
	AccountInfo p3 {"sixteencharname__", "password"};
	AccountInfo p4 {"nameovercharacterlimit", "password"};
	accounts.push_back(p1);
	accounts.push_back(p2);
	accounts.push_back(p3);
	accounts.push_back(p4);

	usernames.push_back("player1");
	usernames.push_back("player2");
	usernames.push_back("sixteencharname__");
	usernames.push_back("nameovercharacterlimit");

	return EXIT_SUCCESS;

}

int Server::run() {
	
	double seconds_per_tick = 1.0f / TICKS_PER_SECOND;
	double time_remaining;
	std::chrono::system_clock::time_point start; 
	std::chrono::system_clock::time_point finish;
	std::chrono::duration<double, std::milli> time_elapsed;
	while(is_running)	{
		
		// State updates should always be sent at (1 / TICKS_PER_SECOND) intervals.
    	// Once (1 / TICKS_PER_SECOND) elapses, update clients and restart loop.
        start = std::chrono::system_clock::now();
        finish = std::chrono::system_clock::now();
        time_elapsed = start - finish;
		while (time_elapsed.count() < seconds_per_tick) {
            
            start = std::chrono::system_clock::now();
            time_elapsed = start - finish;
			time_remaining = seconds_per_tick - time_elapsed.count();
			
			/** Packet validation rules: 
			 * 1. Four possible client packet types: login, logout, input, chat.
			 * 2. First byte of all packet data must be MSG_<TYPE> integer.
			 * 3. Input/logout/chat packets must come from a logged-in IP to be actioned.
			 */
			listen(time_remaining); 
			if (buf[0] != '\0') {
				switch (buf[0] - '0') {  // Convert char byte to int for type checking
					
					case MSG_INPUT:
						std::cout << "[server] input received from client " << inet_ntoa(s_info_client.sin_addr) << std::endl;
						std::cout << buf;
						break;


					case MSG_CHAT:
						std::cout << "[server] chat received from client " << inet_ntoa(s_info_client.sin_addr) << std::endl;
						std::cout << buf;
						break;

					case MSG_LOGIN:
						login();
						break;

					// Logout protocol: same as login, but IP removed instead of recorded.
					case MSG_LOGOUT:
						std::cout << "[server] logout request received from client " << inet_ntoa(s_info_client.sin_addr) << std::endl;
						std::cout << buf;
						break;
					
					default:
						std::cout << "[server] unknown message (" << buf[0] << ") from client " << std::endl;
				}


				// std::cout << inet_ntoa(s_info_client.sin_addr) 
				// 		  << ":" << s_info_client.sin_port
				// 		  << " - " << buf;

				// TODO
			}
		}
		// This should produce mostly consistent timings
        // std::cout << "Time taken: " << time_elapsed.count() << std::endl;   
		
		finish = std::chrono::system_clock::now(); 
				
        // 3
		// TODO
	}

	close(s);
   return EXIT_SUCCESS;	

}

int Server::login() {

	/** 
	 * Login handshake & login message validation: 
	 * 
	 * - Client message buf[1] to buf[USERNAME_MAX_LENGTH + 2] must contain client username.
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
	char str[USERNAME_MAX_LENGTH + 2];
	memcpy(str, &buf[1], USERNAME_MAX_LENGTH + 1);
	std::string username = str;
	username.erase(std::remove(username.begin(), username.end(), ' '), username.end());
	username.erase(std::remove(username.begin(), username.end(), '\n'), username.end());
	
	// Check if username known.
	int slot = -1;
	if (std::find(usernames.begin(), usernames.end(), username) != usernames.end()) {

		std::cout << "[server] login action type " << buf[USERNAME_MAX_LENGTH + 2]
				  << " received for " << username << " from " 
				  << inet_ntoa(s_info_client.sin_addr) << std::endl;

		// Allocate client to a slot if not already allocated.
		slot = allocate_client_connection_slot(username, s_info_client.sin_addr);
		if (slot >= 0) {

			// Debug: print clients
			// std::cout << "[server] logged in or pending clients:" << std::endl;
			// for (int i = 0; i < MAX_PLAYERS; i++ ) {
			// 	if (slots[i].in_use) {
			// 		std::cout << "    " << slots[i].username << std::endl;
			// 	}	
			// }

			// Check buf[USERNAME_MAX_LENGTH + 2] for intended action.
			switch (buf[USERNAME_MAX_LENGTH + 2] - '0') {
				
				// Send challenge packet with salt
				case LOGIN_ACTION_INIT:
					std::cout << "[Server] user " << username << " exists. Sending challenge packet to " << inet_ntoa(s_info_client.sin_addr) << std::endl;	
			
					// Format buffer 
					std::cout << "[server] Slot " << slot << std::endl;
					std::cout << "         salt " << slots[slot].login_salt << std::endl;
					std::cout << "         hash " << slots[slot].login_hash << std::endl;
					std::cout << "     username " << slots[slot].username << std::endl;

					if (sendto(s, buf, recv_len, 0, (struct sockaddr*) &s_info_client, send_len) == -1)	{
						std::cout << "[server] error when sending response packet" << std::endl;
					}	
					break;

				// Check incoming auth hash against actual auth hash 
				case LOGIN_ACTION_AUTH:
					std::cout << "[Server] checking " << username << " credentials." << std::endl;	
					break;
			}
		
		// Deny on error case
		} else {

			// TODO: Denial packet

		}
		
	// Deny if username not known
	} else {
		std::cout << "[Server] username " << username << " does not exist." << std::endl;	

		// TODO: Denial packet

	}


	return EXIT_SUCCESS;
}

int Server::allocate_client_connection_slot(std::string username, in_addr ip) {

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
		client.in_use = true;
		client.addr = ip;
		client.connected = CONN_STATE_PENDING;
		client.index = first_free_slot;
		client.login_salt = salt();
				
		client.login_hash = hash(get_user_details(username).password, client.login_salt);

		client.username = username;

		slots[first_free_slot] = client;

		return first_free_slot;
	} 

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

bool Server::running()  {return is_running;}