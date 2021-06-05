#ifndef SERVER_H
#define SERVER_H

#include "../shared/Shared.h"

// Client message types.
#define MSG_INPUT               1
#define MSG_CHAT                2
#define MSG_LOGIN               3
#define MSG_LOGOUT              4

// Server message types.
#define MSG_UPDATE              8
#define MSG_UPDATE_SLICE        9

// Human reaction time ~200ms, use half that as server tick rate.
#define TICKS_PER_SECOND        10

// Max players server will allow to log in.
#define MAX_PLAYERS             100

// State container for individual clients.
struct ClientState {
    bool connected;
    sockaddr_in addr;  // IP
    char* login_hash;  // SHA256(username + pwd + salt)
    char* account_id;  // Username or email
};

class Server    {
    public:
        Server(const char* address, int port);
        Server();
        ~Server();

        /** Client message handling & world state update
        * 1. Receive incoming packets.
        * 2. Validate packet data and update game state accordingly.
        * 3. Send state update packets back to clients.
        */
        int                    run();

        // Blocking, returns only on packet receival or error.
        // Writes packet message to "buf", if packets exist.
        int                     listen();

        // Blocking for duration of timeout or packet receival or error.
        // Writes packet message to char* buffer "buf", if packets exist.
        int                     timed_listen(double timeout);

        bool                    running();                
        
        // Socket vars.
        int                     s_port;
        const char*             s_addr;
        struct sockaddr_in      s_info_server;
        struct sockaddr_in      s_info_client;
        char                    buf[PACKET_SIZE];
        int                     s, send_len, recv_len;

        // Container for active client states.
        ClientState             slots[MAX_PLAYERS];
    
    private:
        bool                    is_running;        

};


#endif