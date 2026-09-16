#include "socketutil.h"
#include <pthread.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

void error(char* msg){
  perror(msg);
  exit(EXIT_FAILURE);
}

int CreateTCPIPv4Socket(){
  // IPv4, TCP, IP
  return socket(AF_INET, SOCK_STREAM, 0); // Regresa negativo si algo sale mal.
}

/* Función que regresa un apuntador a una conexión aceptada. Reserva memoria en heap y es importante liberarla. */
AcceptedSocket* acceptIncomingConnection(int serverSocketFD){
  struct sockaddr_in clientAddr;
  uint32_t clientAddrSize = sizeof(struct sockaddr_in);
  int clientSocketFD = accept(serverSocketFD, (struct sockaddr*) &clientAddr, &clientAddrSize);

  AcceptedSocket* acceptedSocket = malloc(sizeof(AcceptedSocket));
  acceptedSocket->address = clientAddr;
  acceptedSocket->acceptedSocketFD = clientSocketFD;
  acceptedSocket->acceptedSuccessfully = clientSocketFD > 0;

  if(!acceptedSocket->acceptedSuccessfully)
    acceptedSocket->error = clientSocketFD;
  
  return acceptedSocket;
}

struct sockaddr_in* CreateIPv4Address(char* ip, uint16_t port){
  struct sockaddr_in *addr = malloc(sizeof(struct sockaddr_in)); // IPv4 struct
  addr->sin_family = AF_INET;
  addr->sin_port = htons(port); // htons garantiza que se utilice el Endian correcto para la conexión.
  if(strlen(ip) == 0)
    addr->sin_addr.s_addr = INADDR_ANY; 
  else
    inet_pton(AF_INET,ip,&addr->sin_addr.s_addr); // Convertir ip a unsigned integer y colocarlo en &addr.sin_addr.s_addr.
  return addr;
}

void sendMessage(char* buffer, int socketFD){
  send(socketFD, buffer, strlen(buffer), 0);
}
