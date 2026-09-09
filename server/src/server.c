#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "server.h"
#include "../util/socketutil.h"

/* Función para iniciar el servidor */
void StartServer(uint16_t port, char* ip) {
  // IPv4, TCP, IP

  int serverSocketFD = CreateTCPIPv4Socket();
  if(serverSocketFD < 0) 
    error("Error creando socket.\n");

  struct sockaddr_in* serverAddr = CreateIPv4Address(ip, port);

  int bindResult = bind(serverSocketFD, (struct sockaddr*) serverAddr, sizeof(*serverAddr));

  if(bindResult < 0)
    error("Error en operacion bind.\n"); 

  int listenResult = listen(serverSocketFD, 10);

  if(listenResult < 0)
    error("Error en operacion listen.\n");

  startAcceptingIncomingConnections(serverSocketFD);

  shutdown(serverSocketFD, SHUT_RDWR);
  free(serverAddr);
}

