#ifndef SOCKETUTIL_H_
#define SOCKETUTIL_H_

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <malloc.h>
#include <stdlib.h>

struct sockaddr_in* CreateIPv4Address(char* ip, uint16_t port);

int CreateTCPIPv4Socket();

void error(char* msg);

#endif // SOCKETUTIL_H_
