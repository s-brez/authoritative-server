#ifndef SHARED_H
#define SHARED_H

#define EXIT_SUCCESS                1
#define EXIT_FAILURE                -1
#define SALT_SIZE                   32
#define SERVER_DEFAULT_ADDRESS      "127.0.0.1"
#define SERVER_DEFAULT_PORT         8080
#define USERNAME_MAX_LENGTH         16
#define TICKS_PER_SECOND_SERVER     1
#define TICKS_PER_SECOND_CLIENT     1

// Period after which client will kicked if nothing heard (ms)
#define LAST_HEARD_TIMEOUT          15000

// Client connection states
#define CONN_STATE_DISCONNECTED     0
#define CONN_STATE_CONNECTED        1
#define CONN_STATE_PENDING          2

// Client-to-server packet types
#define MSG_INPUT                   1
#define MSG_CHAT                    2
#define MSG_AUTH                    3

// Client-to-server authenticated message sub-types
#define AUTH_ACTION_INIT            1
#define AUTH_ACTION_VERIFY          2
#define AUTH_ACTION_DISCONNECT      3

// Server-to-client packet type
#define MSG_STATE_UPDATE            1
#define MSG_STATE_UPDATE_SLICE      2
#define MSG_AUTH_CHALLENGE          3
#define MSG_AUTH_SUCCESS            4
#define MSG_FORCE_TERMINATE         5

// Nothing-heard period (seconds) before client auto-disconnect occurs
#define DEFAULT_TIMEOUT             100

// General purpose packet, must not exceed 508 or risk packet data fragmentation
#define PACKET_SIZE                 508

// Smaller packet used only for client input updates.
#define INPUT_PACKET_SIZE           32

// Number of identifier bytes at the start of each packet
#define ID_HEADER_SIZE              ((USERNAME_MAX_LENGTH) + 2)

// Chars per general purpose packet not used for identification
#define FREE_SPACE_PER_PACKET       ((PACKET_SIZE) - (ID_HEADER_SIZE))

// Maximum number of packets a single world state update can be split into.
# define SLICE_COUNT                10

#include <chrono>
#include <thread>
#include <cstring>
#include <ctime>
#include <string>
#include <netdb.h>
#include <stdio.h>
#include <iostream>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <algorithm>
#include <sys/time.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <openssl/sha.h>

// Returns random alphanumeric string of size SALT_SIZE
std::string salt();

// openssl SHA256 lib method
std::string SHA256(std::string string);

#endif