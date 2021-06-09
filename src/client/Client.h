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

        int                         run();

        int                         listen(double timeout);

        bool                        set_account_details();

        int                         send_auth_init_message();
        int                         send_auth_verify_message();

        bool                        running();
        int                         c_port;
        const char*                 c_addr;
        
    private:
        struct sockaddr_in          s_info_server;
        int                         s, send_len, recv_len;
        int                         connection;
        bool                        is_running;
        char                        buf[PACKET_SIZE];
	    char                        msg[PACKET_SIZE];
        std::string                 hash;
        std::string                 salt;
        std::string                 username;
        std::string                 password;

        
        
        
};


#endif