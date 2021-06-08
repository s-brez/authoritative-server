#ifndef SHARED_H
#define SHARED_H

#define EXIT_SUCCESS                1
#define EXIT_FAILURE                -1

// Cryptography vars, dont change
#define CHUNK_SIZE 64
#define TOTAL_LEN_LEN 8
#define SALT_SIZE                   26

#define SERVER_DEFAULT_ADDRESS      "127.0.0.1"
#define SERVER_DEFAULT_PORT         8080

// Must not exceed 508
#define PACKET_SIZE                 256

#define USERNAME_MAX_LENGTH         16

#include <chrono>
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


// Returns ms timestamp as int
std::string salt();

// Returns SHA256 hash of pwd and salt
std::string hash(std::string pwd, std::string salt);

// SHA256 implementation
void calc_sha_256(uint8_t hash[32], const uint8_t *input, size_t len);


#endif