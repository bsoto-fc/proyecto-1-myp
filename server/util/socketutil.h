#ifndef SOCKETUTIL_H_
#define SOCKETUTIL_H_

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <malloc.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>

typedef struct {
  int acceptedSocketFD;
  struct sockaddr_in address;
  int error;
  bool acceptedSuccessfully;
} AcceptedSocket;

struct sockaddr_in* CreateIPv4Address(char* ip, uint16_t port);

int CreateTCPIPv4Socket();

void error(char* msg);

AcceptedSocket* acceptIncomingConnection(int serverSocketFD);

void* receiveAndPrintIncomingData(void* socketFD);

void startAcceptingIncomingConnections(int serverSocketFD);

void receiveAndPrintIncomingDataOnSeparateThread(AcceptedSocket* pSocket); 

#endif // SOCKETUTIL_H_
