#ifndef SERVER_H
#define SERVER_H

#include "../shared/Shared.h"

// Human reaction time ~200ms, use half that as server tick rate
#define TICKS_PER_SECOND      10

class Server    {
    public:
        Server(const char* address, int port);
        Server();
        ~Server();

        // Blocking, returns only on packet receival or error.
        int                     listen(Packet packet_buffer);

        // Blocking, returns on packet receival, error, or when timeout elapses.
        int                     timed_listen(Packet packet_buffer, int timeout);

        bool                    running();                
        int                     s_port;
        const char*             s_addr;
        struct sockaddr_in      s_info_server;
        struct sockaddr_in      s_info_client;
        int                     s, send_len, recv_len;
    
    private:
        bool                    is_running;        

};


#endif