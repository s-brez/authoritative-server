#include "Client.h"

int main(int argc, char *argv[]) {
    
    // Attempt to init client.
    Client client;
    if (argc == 1) {
        client = Client(CLIENT_DEFAULT_ADDRESS, CLIENT_DEFAULT_PORT);
    } else if (argc == 2) {
        client = Client(argv[1], CLIENT_DEFAULT_PORT);
    } else if (argc == 3) {
        client = Client(argv[1], std::stoi(argv[2]));
    } else {
        client = Client(argv[1], std::stoi(argv[2]));
    }

    while (client.running()) {

            // Buffer for incoming packets

            // Write packet data to buffer

            // Action packet message

    }
    return EXIT_SUCCESS;

};


Client::Client(const std::string& address, int port)  {
    
    c_port = port;
    c_addr = address;
    char decimal_port[16];

    // Format address and port
    snprintf(decimal_port, sizeof(decimal_port), "%d", c_port);
    decimal_port[sizeof(decimal_port) / sizeof(decimal_port[0]) - 1] = '\0';
    
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;
    
    // Get address info
    int r(getaddrinfo(address.c_str(), decimal_port, &hints, &c_addrinfo));
    if(r != 0 || c_addrinfo == NULL)    {
        std::cout << "[client] Invalid address or port:  " << address << ":" << decimal_port << std::endl;
        is_running = false;
    } else { is_running = true; }
    
    // Create socket
    c_socket = socket(c_addrinfo->ai_family, SOCK_DGRAM | SOCK_CLOEXEC, IPPROTO_UDP);
    if(c_socket == -1)  {
        freeaddrinfo(c_addrinfo);
        std::cout << "[client] Failed to create socket for: " << address << ":" << decimal_port << std::endl;
        is_running = false;
    } else { is_running = true; }
}

Client::Client()    {
    is_running = false;
}

Client::~Client()   {
    freeaddrinfo(c_addrinfo);
    close(c_socket);
}


bool Client::running()  {return is_running;}
const std::string Client::get_addr()  {return c_addr;}
const int Client::get_port()  {return c_port;}
const int Client::get_socket()    {return c_socket;}

