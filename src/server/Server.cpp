#include "Server.h"


int main(int argc, char *argv[])	{
	
    Server server = Server(SERVER_DEFAULT_ADDRESS, SERVER_DEFAULT_PORT);

	/** Server workflow
	 * 1. Receive incoming packets.
	 * 2. Validate packet data and update game state accordingly.
	 * 3. Send state update packets back to clients.
	 * 
	 * State updates should always be sent at (1 / TICKS_PER_SECOND) intervals.
	 * Once (1 / TICKS_PER_SECOND) elapses, update clients and restart loop.
	 */
	double seconds_per_tick = 1.0f / TICKS_PER_SECOND;
	while(server.running())	{
		
        std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
        std::chrono::system_clock::time_point finish = std::chrono::system_clock::now();
        std::chrono::duration<double, std::milli> time_elapsed = start - finish;
		double time_remaining;
        
		while (time_elapsed.count() < seconds_per_tick) {
            
            start = std::chrono::system_clock::now();
            time_elapsed = start - finish;
			time_remaining = seconds_per_tick - time_elapsed.count();
			
			/** Packet validation rules: 
			 * 1. Four possible client packet types: login, logout, input, chat.
			 * 2. First byte of all packets' data must be MSG_<TYPE> integer.
			 * 3. Input message packets must come from a logged-in IP to be actioned.
			 */
			server.timed_listen(time_remaining);
			if (server.buf[0] != '\0') {
				switch (server.buf[0] - '0') {  // Convert char byte to int for type checking
					
					case MSG_INPUT:
						break;


					case MSG_CHAT:
						break;

					/** Login protocol:
					 * 1. Clients send login request message.
					 * 2. Server responds with ack, and salt.
					 * 3. Client sends hash of username and password and salt.
					 * 4. If hash matches, client IP recorded as logged in until timeout or logout.
					 */
					case MSG_LOGIN:
						break;

					// Logout protocol: same as login, but IP removed instead of recorded.
					case MSG_LOGOUT:
						break;
					
					default:
						std::cout << "[server] unknown message type (" << server.buf[0] << ") from client ";
				}


				std::cout << inet_ntoa(server.s_info_client.sin_addr) 
						  << ":" << server.s_info_client.sin_port
						  << " - " << server.buf;

				// TODO
			}
		}
		// This should produce mostly consistent timings
        // std::cout << "Time taken: " << time_elapsed.count() << std::endl;   
		
		finish = std::chrono::system_clock::now(); 
				
        // 3
		// TODO
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
	s_info_server.sin_addr.s_addr = inet_addr(address);  // Specified address
	// s_info.sin_addr.s_addr = htonl(INADDR_ANY);	// Localhost

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

int Server::listen() {
	
	// Clear buffer and receive message.
	memset(buf,'\0', PACKET_SIZE);
	recv_len = 0;
	if ((recv_len = recvfrom(s, buf, PACKET_SIZE, 0, (struct sockaddr *) &s_info_client,  (socklen_t*)&send_len)) == -1)	{
		std::cout << "[server] error when reading incoming packet" << std::endl;
	}

	std::cout << inet_ntoa(s_info_client.sin_addr) << std::endl;

	// Send response message.
	if (sendto(s, buf, recv_len, 0, (struct sockaddr*) &s_info_client, send_len) == -1)	{
		std::cout << "[server] error when sending response packet" << std::endl;
	}	

	return EXIT_SUCCESS;
} 

int Server::timed_listen(double timeout) {

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

bool Server::running()  {return is_running;}