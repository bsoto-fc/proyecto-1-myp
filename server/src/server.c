#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

#include "server.h"
#include "../util/socketutil.h"
#include "../util/jsonutil.h"
#include "users.h"

bool serverRunning = false;
int serverSocketFD = -1;
pthread_t keyboardID;
UserList userList;

// TO DO: Pasarlo a pruebas unitarias.
/* Función que añade usuarios de prueba. */
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
}

void ShutdownServer() {
    serverRunning = false;
    shutdown(serverSocketFD, SHUT_RDWR);
}

/* Función que recibe comandos del teclado. */
void* ReceiveKeyboardCommands() {
  char buff[100];
  
  while(serverRunning) {
      if(fgets(buff, sizeof(buff), stdin) == NULL) {
          break;
      }
      printf("Comando: %s",buff);
      if(strcmp(buff, "exit\n") == 0) {
          ShutdownServer();
          break;
      }
      if(strcmp(buff, "dummy\n") == 0)
          AddDummyUsers();
      if(strcmp(buff, "print\n") == 0)
          printf("User list: %s",GenerateUserListJSON(&userList));
  }
  
  return NULL;
} 

/* Función que crea un nuevo hilo de ejecución para recibir comandos del teclado. */
void CreateStdinThreadForInput(){
  pthread_create(&keyboardID, NULL, ReceiveKeyboardCommands, NULL);
}

/* Función que recibe peticiones JSON y manda respuestas al cliente. */
void* receiveAndSendResponse(void* data) {
  printf("[SERVER]: Cliente conectado.\n");
  char buffer[1024];

  struct thread_info* info = data;

  bool receiving = true;
  UserEntry user;
  bool authenticated = false;
  
  while(receiving) {
    ssize_t amountReceived = recv(info->socketFD, buffer, sizeof(buffer), 0);
    if(amountReceived > 0) {
      // To do: Implementar un manejo ante buffer overflows (mensajes más largos que 1024).
      buffer[amountReceived-1] = '\0';
      printf("[SERVER]: Cliente mando: \"%s\"\n",buffer);
      if(!authenticated) {
          if(!StartFirstTimeAuthentication(buffer, &userList, info->socketFD, &user)){
              printf("[SERVER]: Error al autenticar usuario.\n");
              close(info->socketFD);
              receiving = false;
          } else
              authenticated = true;
      } else if(!determineJSONResponse(buffer, &userList, info->socketFD,user.username)) {
          printf("[SERVER]: Error al procesar petición del usuario %s\n",user.username);
          DisconnectUser(&userList, user.username, info->socketFD);
          authenticated = false;
          receiving = false;
      }
    }
    if(amountReceived == 0) {
        if(!strcmp(user.username, "") == 0) {
          DisconnectUser(&userList, user.username, info->socketFD);
          authenticated = false;
          receiving = false;
        } else {
            close(info->socketFD);
            receiving = false;
        }
    }
  }
  printf("[SERVER]: Cliente desconectado.\n");
  free(info);
  return NULL;
}

/* Función crea un hilo de ejecución para recibir peticiones JSON y mandar respuestas al cliente. */
void receiveAndSendResponseOnSeparateThread(AcceptedSocket* pSocket) {
  pthread_t id;
  struct thread_info* info = malloc(sizeof(struct thread_info));
  info->socketFD = pSocket->acceptedSocketFD;
  pthread_create(&id, NULL, receiveAndSendResponse, info);
}

/* Función para empezar a recibir respuestas de los clientes. */
void startAcceptingIncomingConnections(int serverSocketFD) {
  while(serverRunning) {
    AcceptedSocket* clientSocket = acceptIncomingConnection(serverSocketFD);
    if(clientSocket == NULL){
        if(!serverRunning)
            break;
        perror("accept");
        continue;
    }
    receiveAndSendResponseOnSeparateThread(clientSocket);
  }
  printf("[SERVER]: Shutting down...\n");
}

/* Función para iniciar el servidor */
void StartServer(uint16_t port, char* ip) {
  serverSocketFD = CreateTCPIPv4Socket();
  if(serverSocketFD < 0) 
    error("Error creando socket.\n");

  // Evitar error "Address already in use."
  int opt = 1;
  if (setsockopt(serverSocketFD, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
      perror("setsockopt");
      exit(EXIT_FAILURE);
  }
  struct sockaddr_in* serverAddr = CreateIPv4Address(ip, port);
  
  if(bind(serverSocketFD, (struct sockaddr*) serverAddr, sizeof(*serverAddr)) < 0) {
    close(serverSocketFD);
    free(serverAddr);
    error("Error al crear lista de usuarios.\n");
  } 

  if(listen(serverSocketFD,10) < 0) {
    close(serverSocketFD);
    free(serverAddr);
    error("Error al crear lista de usuarios.\n");
  } 

  if(!InitUserList(&userList)) {
    close(serverSocketFD);
    free(serverAddr);
    error("Error al crear lista de usuarios.\n");
  } 

  serverRunning = true;

  signal(SIGINT,ShutdownServer);
  
  CreateStdinThreadForInput();

  startAcceptingIncomingConnections(serverSocketFD);

  pthread_join(keyboardID, NULL);
  
  close(serverSocketFD);

  free(serverAddr);
  DestroyUserList(&userList);
}

