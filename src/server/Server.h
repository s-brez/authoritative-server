#ifndef SERVER_H
#define SERVER_H

#include "../shared/Shared.h"

#define TICKS_PER_SECOND      30

class Server    {
    public:
        Server(const char* address, int port);
        Server();
        ~Server();
        bool                    running();                
        int                     listen(char *buf, int max_size);
        int                     s_port;
        const char*             s_addr;
        struct sockaddr_in      s_info_server;
        struct sockaddr_in      s_info_client;
        int                     s, send_len, recv_len;
    
    private:
        bool                    is_running;        

};


#endif