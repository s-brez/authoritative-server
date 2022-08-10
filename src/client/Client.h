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

        int                         listen();

        bool                        set_account_details();

        int                         send_auth_init_packet();
        int                         send_auth_verify_packet();

        int                         process_state_update_packet();
        int                         send_input();

        bool                        running();
        int                         c_port;
        const char*                 c_addr;

        int                         serialize_input();
        
    private:
        struct sockaddr_in          s_info_server;
        int                         s, send_len, recv_len;
        int                         connection;
        bool                        should_run;
        char                        buf[PACKET_SIZE];
	    char                        msg[PACKET_SIZE];
        char                        imsg[INPUT_PACKET_SIZE];
        std::string                 hash;
        std::string                 salt;
        std::string                 username;
        std::string                 password;

        char                        i_buf[INPUT_PACKET_SIZE];

        
        
        
};


#endif