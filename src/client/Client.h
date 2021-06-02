#ifndef CLIENT_H
#define CLIENT_H

#include "../shared/Shared.h"

#define CLIENT_DEFAULT_ADDRESS     "127.0.0.1"
#define CLIENT_DEFAULT_PORT        8081

class Client
{
    public:
        Client();
        Client(const std::string& address, int port);
        ~Client();
        

        int                 send(const char *msg, size_t size);       

        bool                running();
        const int           get_socket();
        const int           get_port();
        const std::string   get_addr();

    private:
        int                 c_socket;
        int                 c_port;
        std::string         c_addr;
        struct addrinfo*    c_addrinfo;
        bool                is_running;
        
        
};


#endif