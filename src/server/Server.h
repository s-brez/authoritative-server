#ifndef SERVER_H
#define SERVER_H

#include "../shared/Shared.h"

#include <vector>

// Client-to-server message types
#define MSG_INPUT                   1
#define MSG_CHAT                    2
#define MSG_LOGIN                   3
#define MSG_LOGOUT                  4

// Server-to-client message types
#define MSG_UPDATE                  8
#define MSG_UPDATE_SLICE            9

// Client-to-server login message sub-types
#define LOGIN_ACTION_INIT           1
#define LOGIN_ACTION_AUTH           2

// Client connection states
#define CONN_STATE_CONNECTED        1
#define CONN_STATE_PENDING          2

// Human reaction time ~200ms, use half that as server tick rate.
#define TICKS_PER_SECOND        10

// Max players server will allow to log in.
#define MAX_PLAYERS             100

// State container for individual client login status.
struct ClientState {
    bool in_use = false;
    int connected;                  // STATE_CONNECTED or STATE_PENDING
    in_addr addr;                   // IP
    std::string login_salt;
    std::string login_hash;           // SHA256(pwd + salt)
    std::string username;
    int index;                      // Location of this struct in ClientState slots array
};

struct AccountInfo {
    std::string username;
    std::string password;
};

class Server    {
    public:
        Server(const char* address, int port);
        Server();
        ~Server();

        /**
         * 1. Receive incoming packets.
         * 2. Validate packet data and update game state accordingly.
         * 3. Send state update packets back to clients.
         */
        int                         run();

        /** 
         * Attempts to receive packets.
         * Blocking for duration of timeout, or packet receival, or error.
         * Writes packet message to char* buffer "buf", if packets exist.
         */
        int                         listen(double timeout);

        /** Login message handler:
         * 1. Client sends login request message.
         * 2. Server responds with ack and salt if client username known.
         * 3. Client sends hash of username + password + salt.
         * 4. If hash matches, client IP logged in until timeout or logout.
         * 
         * Also facilitates logout at step 3.
         */
        int                         login();

        /**
         * Check and update the clients connection status.
         * If client not already allocated to slot, alocate to slot if free slot exists.
         */
        int                         allocate_client_connection_slot(std::string username, in_addr ip);

        bool                        running();       

        // Fetch known account names from DB
        int                         update_known_accounts();         
        
        // Return account info matching given username.
        AccountInfo                 get_user_details(std::string username);

        // Socket vars.
        int                         s_port;
        const char*                 s_addr;
        struct sockaddr_in          s_info_server;
        struct sockaddr_in          s_info_client;
        char                        buf[PACKET_SIZE];
        int                         s, send_len, recv_len;

        // Container for active client states.
        ClientState                 slots[MAX_PLAYERS];

        
    
    private:
        int                         total_connected_clients;
        bool                        is_running;        
        std::vector<std::string>    usernames;
        std::vector<AccountInfo>    accounts;

};


#endif