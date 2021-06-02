#ifndef SHARED_H
#define SHARED_H

// #define _GLIBCXX_USE_CXX11_ABI 0
#define EXIT_SUCCESS                1
#define EXIT_FAILURE                -1

#define SERVER_DEFAULT_ADDRESS      "127.0.0.1"
#define SERVER_DEFAULT_PORT         8080

#define MSG_BUFFER_MAX_SIZE         256

#include <string.h>
#include <unistd.h>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <chrono>


#endif