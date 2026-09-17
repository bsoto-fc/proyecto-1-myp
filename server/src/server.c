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
#include "../util/jsonutil.h"
#include "users.h"

bool serverRunning = false;
UserList userList;

// TO DO: Pasarlo a pruebas unitarias.
void AddDummyUsers() {
  printf("Añadiendo usuarios.\n"); 
  AddUser(&userList, "s1", AWAY, 1);
  AddUser(&userList, "s12", AWAY, 12);
  AddUser(&userList, "s123", AWAY, 123);

  UserEntry s1;
  GetUser(&userList, "s1", &s1);

  UserEntry s2;
  GetUser(&userList, "s12", &s2);

  UserEntry s3;
  GetUser(&userList, "s123", &s3);

  printf("Usuarios:\n");
  printf("%s,%d,%d\n",s1.username,s1.user.status,s1.user.clientFD);
  printf("%s,%d,%d\n",s2.username,s2.user.status,s2.user.clientFD);
  printf("%s,%d,%d\n",s3.username,s3.user.status,s3.user.clientFD);
}

void* ReceiveKeyboardCommands() {
  bool receivingInput = true;
  
  char buff[100];
  
  while(receivingInput) {
    fgets(buff, sizeof(buff), stdin);
    printf("Comando: %s",buff);
    if(strcmp(buff, "exit\n") == 0) {
      receivingInput = false;
      serverRunning = false;
    }
    if(strcmp(buff, "dummy\n") == 0)
      AddDummyUsers();
    if(strcmp(buff, "print\n") == 0)
      printf("User list: %s",GenerateUserListJSON(&userList));
  }
  
  return NULL;
} 

void CreateStdinThreadForInput(){
  pthread_t id;
  pthread_create(&id, NULL, ReceiveKeyboardCommands, NULL);
}

void* receiveAndSendResponse(void* data) {
  char buffer[1024];

  struct thread_info* info = data;

  bool receiving = true;
  UserEntry user;
  
  while(receiving) {
    ssize_t amountReceived = recv(info->socketFD, buffer, sizeof(buffer), 0);
    if(amountReceived > 0) {
      // To do: Implementar un mejor manejo ante buffer overflows.
      if(amountReceived > 1024)
        error("Buffer overflow.");
      buffer[amountReceived-1] = '\0';
      printf("[SERVER]: Cliente mando: \"%s\"\n",buffer);
      // TO DO: Enviar respuesta a los demás clientes,
      if(!determineJSONResponse(buffer, &userList, info->socketFD)) {
        printf("[SERVER]: Error al autenticar usuario.\n");
        receiving = false;
      }
    }
    if(amountReceived == 0)
      receiving = false;
  }
  DeleteUser(&userList, user.username);
  close(info->socketFD);
  free(info);
  printf("[SERVER]: Se desconecto el usuario %s.\n",user.username);
  return NULL;
}

void receiveAndSendResponseOnSeparateThread(AcceptedSocket* pSocket) {
  pthread_t id;
  struct thread_info* info = malloc(sizeof(struct thread_info));
  info->socketFD = pSocket->acceptedSocketFD;
  pthread_create(&id, NULL, receiveAndSendResponse, info);
}

void startAcceptingIncomingConnections(int serverSocketFD) {
  while(serverRunning) {
    AcceptedSocket* clientSocket = acceptIncomingConnection(serverSocketFD);
    // TO DO: Añadir usuario a la lista de usuarios.
    // 
    /* receiveAndPrintIncomingDataOnSeparateThread(clientSocket); */
    receiveAndSendResponseOnSeparateThread(clientSocket);
  }
  printf("[SERVER]: Shutting down...");
}

/* Función para iniciar el servidor */
void StartServer(uint16_t port, char* ip) {

  int serverSocketFD = CreateTCPIPv4Socket();
  if(serverSocketFD < 0) 
    error("Error creando socket.\n");

  struct sockaddr_in* serverAddr = CreateIPv4Address(ip, port);

  if(bind(serverSocketFD, (struct sockaddr*) serverAddr, sizeof(*serverAddr)) < 0)
    error("Error en operacion bind.\n"); 

  if(listen(serverSocketFD,10) < 0)
    error("Error en operacion listen.\n");

  serverRunning = true;

  CreateStdinThreadForInput();

  if(InitUserList(&userList) == false) 
    error("Error al crear lista de usuarios.\n");

  startAcceptingIncomingConnections(serverSocketFD);

  shutdown(serverSocketFD, SHUT_RDWR);
  free(serverAddr);

  DestroyUserList(&userList);
}

