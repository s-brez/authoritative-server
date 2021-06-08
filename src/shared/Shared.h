#ifndef SHARED_H
#define SHARED_H

#define EXIT_SUCCESS                1
#define EXIT_FAILURE                -1

#define SALT_SIZE                   32

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

#include <openssl/sha.h>

// Returns ms timestamp as int
std::string salt();

// SHA256 hashes pwd and salt
std::string hash(std::string pwd, std::string salt);

// openssl SHA256 lib
bool SHA256(void* input, unsigned long length, unsigned char* md);

#endif