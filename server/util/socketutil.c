#include "socketutil.h"
#include <pthread.h>
#include <unistd.h>

void error(char* msg){
  perror(msg);
  exit(EXIT_FAILURE);
}

int CreateTCPIPv4Socket(){
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

void startAcceptingIncomingConnections(int serverSocketFD) {
  while(true) {
    AcceptedSocket* clientSocket = acceptIncomingConnection(serverSocketFD);
    receiveAndPrintIncomingDataOnSeparateThread(clientSocket);
  }
}

struct thread_info{
  int socketFD;
};

void receiveAndPrintIncomingDataOnSeparateThread(AcceptedSocket* pSocket) {
  pthread_t id;
  struct thread_info* info = malloc(sizeof(struct thread_info));
  info->socketFD = pSocket->acceptedSocketFD;
  pthread_create(&id, NULL, receiveAndPrintIncomingData, info);
}

void* receiveAndPrintIncomingData(void* data) {
  char buffer[1024];

  struct thread_info* info = data;
  
  while(true) {
      ssize_t amountReceived = recv(info->socketFD, buffer, sizeof(buffer), 0);
      if(amountReceived > 0) {
          // To do: Implementar un mejor manejo ante buffer overflows.
          if(amountReceived > 1024)
              error("Buffer overflow.");
          buffer[amountReceived] = 0;
          printf("Cliente mando: \"%s\"",buffer);
      }
      if(amountReceived == 0)
          break;
  }

  close(info->socketFD);
  free(info);
  return NULL;
}
