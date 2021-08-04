/*
*	C++ HTTP Server
*	Copyright (C) Mohammed Zayyad
*/


#if defined(_WIN32)
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif

// Header files for windows

#include <winsock2.h>
#include <ws2tcpip.h>

// Needed for winsock library 
#pragma comment(lib, "ws2_32.lib")

#else 

// Header files for unix-based os

#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>

#endif

// Required for used by both winsock and berkely sockets

#if defined(_WIN32)

#define IsValidSocket(s) ((s) != INVALID_SOCKET)
#define CloseSocket(s) closesocket(s)
#define GetErrorNo() (WSAGetLastError)
#else

#define IsValidSocket(s) ((s) >= 1)
#define CloseSocket(s) close(s)
#define GetErrorNo() (errno)
#define SOCKET int

#endif

// Global header files

#include <stdio.h>
#include <string.h>
#include <stdlib.h>


