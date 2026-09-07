#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "server.h"
#include "../util/socketutil.h"

void StartServer(uint16_t port, char* ip) {
  // IPv4, TCP, IP

  int serverSocketFD = CreateTCPIPv4Socket();
  if(serverSocketFD < 0) {
    error("Error creando socket.\n");
  }

  struct sockaddr_in* serverAddr = CreateIPv4Address(ip, port);

  int bindResult = bind(serverSocketFD, (struct sockaddr*) serverAddr, sizeof(*serverAddr));

  if(bindResult == 0)
    printf("Operacion bind fue exitosa!\n");
  else 
    error("Error en operacion bind.\n"); 

  int listenResult = listen(serverSocketFD, 10);

  if(listenResult == 0)
    printf("Listening en port %d\n",port);
  else
    error("Error en operacion listen.\n");

  struct sockaddr_in clientAddr;
  uint32_t clientAddrSize = sizeof(struct sockaddr_in);
  int clientSocketFD = accept(serverSocketFD, (struct sockaddr*) &clientAddr, &clientAddrSize);

  char buffer[1024];
  recv(clientSocketFD, buffer, sizeof(buffer), 0);

  printf("Respuesta fue %s\n",buffer);
}

