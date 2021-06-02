#ifndef SERVER_H
#define SERVER_H

#include "../shared/Shared.h"

#define TICKS_PER_SECOND      30

class Server    {
    public:
        Server();
        Server(const std::string address, int port);
        ~Server();
                
        /* Listens for packets until packet received (with no timeout).
        * msg              Message buffer to write packet data to.
        * max_size         Maximum size of message buffer.
        * returns:         Total bytes read or -1 on error case.
        */
        int                     listen(char *msg, size_t max_size);

        bool                    running();
        const int               get_socket();
        const int               get_port();
        const std::string       get_addr();

    private:
        int                     s_socket;
        int                     s_port;
        std::string             s_addr;
        struct addrinfo*        s_addrinfo;
        bool                    is_running;
        
};


#endif