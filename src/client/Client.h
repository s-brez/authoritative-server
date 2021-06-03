#ifndef CLIENT_H
#define CLIENT_H

#include "../shared/Shared.h"

#define CLIENT_DEFAULT_ADDRESS     "127.0.0.1"
#define CLIENT_DEFAULT_PORT        8080

class Client
{
    public:
        Client();
        Client(const char* address, int port);
        ~Client();

        int                     send(char *buf, char *msg, int max_size);
        bool                    running();
        int                     c_port;
        const char*             c_addr;
        
        struct sockaddr_in      s_info_server;
        int                     s, send_len, recv_len;

    private:
        bool                    is_running;
        
        
};


#endif