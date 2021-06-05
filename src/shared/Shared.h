#ifndef SHARED_H
#define SHARED_H

#define EXIT_SUCCESS                1
#define EXIT_FAILURE                -1

#define SERVER_DEFAULT_ADDRESS      "127.0.0.1"
#define SERVER_DEFAULT_PORT         8080

// Must not exceed 508
#define PACKET_SIZE                 256

#include <chrono>
#include <netdb.h>
#include <stdio.h>
#include <iostream>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>


struct Packet {
    char* msg[PACKET_SIZE];
    in_addr sender;
    int timestamp;
    int msg_len;        
    int idx;            // Sequence number, derive from message
};

#endif