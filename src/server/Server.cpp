#include "Server.h"

// Parameters
int main(int argc, char *argv[]) {
    
    // Init server.
    Server server;
    if (argc == 1) {
        server = Server(SERVER_DEFAULT_ADDRESS, SERVER_DEFAULT_PORT);
    } else if (argc == 2) {
        server = Server(argv[1], SERVER_DEFAULT_PORT);
    } else if (argc == 3) {
        server = Server(argv[1], std::stoi(argv[2]));
    } else {
        server = Server(argv[1], std::stoi(argv[2]));
    }
   
    // Buffer to hold incoming packets
    // int* packet_buffer;

    double seconds_per_tick = 1.0f / TICKS_PER_SECOND;
    while (server.running()) {
	                
        // Only action packets for (1 / UPDATES_PER_SECOND) seconds before pushing state updates.
        // Once this period elapses, push state update packets to all clients and restart time cycle
        std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
        std::chrono::system_clock::time_point finish = std::chrono::system_clock::now();
        std::chrono::duration<double, std::milli> processing_time = start - finish;
        while (processing_time.count() < seconds_per_tick) {
            
            start = std::chrono::system_clock::now();
            processing_time = start - finish;
            std::cout << processing_time.count() << std::endl;    

            // Read and action new packets
            // while (server.listen()) {

                // Buffer for incoming packets

                // Write packet data to buffer

                // Action packet message

            // }
        }
        finish = std::chrono::system_clock::now();

        // Update client last-heard values

        // Push state update packets back to clients

    }
    

    return EXIT_SUCCESS;
};

Server::Server(const std::string address, int port) {
    
    is_running = false;
    s_port = port;
    s_addr = address;
    char decimal_port[16];
    
    // Format address and port
    snprintf(decimal_port, sizeof(decimal_port), "%d", port);
    decimal_port[sizeof(decimal_port) / sizeof(decimal_port[0]) - 1] = '\0';
    
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;
    
    // Get address
    int r(getaddrinfo(address.c_str(), decimal_port, &hints, &s_addrinfo));
    if(r != 0 || s_addrinfo == NULL)    {
        std::cout << "[server] Invalid address or port: " << address << ":" << decimal_port << std::endl;
        is_running = false;
    } else { is_running = true; }
    
    // Create socket
    s_socket = socket(s_addrinfo->ai_family, SOCK_DGRAM | SOCK_CLOEXEC, IPPROTO_UDP);
    if(s_socket == -1)  {
        freeaddrinfo(s_addrinfo);
        std::cout << "[server] Failed to create socket: " << address << ":" << decimal_port << std::endl;
        is_running = false;
    } else { is_running = true; }
    
    // Bind socket to address
    r = bind(s_socket, s_addrinfo->ai_addr, s_addrinfo->ai_addrlen);
    if(r != 0)  {
        freeaddrinfo(s_addrinfo);
        close(s_socket);
        std::cout << "[server] Failed to bind socket: " << address << ":" << decimal_port << std::endl;
        is_running = false;
    } else { is_running = true; }
}

Server::Server() {
    is_running = false;
}

Server::~Server()   {
    freeaddrinfo(s_addrinfo);
    close(s_socket);
}

bool Server::running()  {return is_running;}
const int Server::get_socket()  {return s_socket;}
const int Server::get_port()    {return s_port;}
const std::string Server::get_addr() {return s_addr;}

int Server::listen(char *msg, size_t max_size)    {
    return ::recv(s_socket, msg, max_size, 0);
}

