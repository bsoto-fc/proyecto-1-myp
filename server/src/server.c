#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "server.h"
#include "../util/socketutil.h"

_Bool serverRunning = false;

void* ReceiveKeyboardCommands() {
  _Bool receivingInput = true;
  
  char buff[100];
  
  while(receivingInput) {
    fgets(buff, sizeof(buff), stdin);
    printf("Comando: %s",buff);
    if(strcmp(buff, "exit") == 0)
      serverRunning = false;
  }
  
  return NULL;
} 

void CreateStdinThreadForInput(){
  pthread_t id;
  pthread_create(&id, NULL, ReceiveKeyboardCommands, NULL);
}

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

  serverRunning = true;

  CreateStdinThreadForInput();

  startAcceptingIncomingConnections(serverSocketFD);

  shutdown(serverSocketFD, SHUT_RDWR);
  free(serverAddr);
}

