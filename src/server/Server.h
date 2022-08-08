#ifndef SERVER_H
#define SERVER_H

#include "../shared/Shared.h"

#include <vector>

// Human reaction time ~200ms, use half that as server tick rate.
// #define TICKS_PER_SECOND        10

#define TICKS_PER_SECOND        1   // debug tick rate

// Max players server will allow to log in.
#define MAX_PLAYERS             10

// State container for individual client login status.
struct ClientState {
    bool in_use = false;
    int connected;                  // Connection state
    sockaddr_in addr;               
    std::string login_salt;
    std::string login_hash;         // SHA256(pwd + salt)
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

        /** Authenticated message handler:
         * 1. Client sends login/init request message.
         * 2. Server responds with ack and salt if client username known.
         * 3. Client sends hash of username + password + salt.
         * 4. If hash matches, client IP logged in until timeout or logout.
         * 5. Client also uses this protocol to logout
         */
        int                         handle_auth_messages();

        /**
         * Check and update the clients connection status.
         * If client not already allocated to slot, alocate to slot if free slot exists.
         */
        int                         allocate_client_connection_slot(std::string username, sockaddr_in address);

        bool                        verify_auth_hash(std::string actual, char* test);

        /**
         * Send respective packet to client.
         * Update local ClientState to reflect what stage of auth client is at.
         */
        int                         send_challenge_message(ClientState client);
        int                         send_success_message(ClientState client);
        int                         send_terminate_message(ClientState client);

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
        char                        msg[PACKET_SIZE];
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