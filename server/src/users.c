#include "users.h"
#include "../util/socketutil.h"
#include "socketutil.h"
#include <cjson/cJSON.h>
#include <pthread.h>
#include <string.h>
#include <stdio.h>

#define USERNAME_MAX 64

/* Boilerplate para comparar en diccionario. */
int user_compare(const void *a, const void *b, void *udata) {
  const UserEntry *ua = a;
  const UserEntry *ub = b;
  return strcmp(ua->username, ub->username);
}

/* Boilerplate de hash. */
uint64_t user_hash(const void *item, uint64_t seed0, uint64_t seed1) {
  const UserEntry *user = item;
  return hashmap_sip(user->username, strlen(user->username), seed0, seed1);
}

/* Inicializa la lista de usuarios del servidor. Regresa false si hubo algún error. Reserva memoria
 * en heap y debe ser destruida con la función DestroyUserList().*/
bool InitUserList(UserList* list){
  list->userList = hashmap_new(sizeof(UserEntry), 0, 0, 0, user_hash, user_compare, NULL, NULL);
  if(list->userList == NULL)
    return false;
  if(pthread_mutex_init(&list->mutexLock, NULL) != 0){
    hashmap_free(list->userList);
    list->userList = NULL;
    return false;
  }
  return true;
}

/* Destruye la lista de usuarios y libera la memoria reservada. Utilzar al final de la ejecución del programa. */
void DestroyUserList(UserList* userList){
  if(userList == NULL)
    return;
  pthread_mutex_destroy(&userList->mutexLock);
  hashmap_free(userList->userList);
  userList->userList = NULL;
} 

/* Crea usuario y lo añade a la lista de usuarios. Regresa false si existe algún error. */
bool AddUser(UserList* userList, const char* username, int status, int clientFD) {
  if(userList == NULL || username == NULL)
    return false;
  if(strlen(username) >= USERNAME_MAX)
    return false;
  UserEntry entry = { .user = {.status = status, .clientFD = clientFD} };
  strcpy(entry.username, username);
  pthread_mutex_lock(&userList->mutexLock);
  const UserEntry* existingUser = hashmap_get(userList->userList,&entry);
  if(existingUser != NULL) {
    printf("[SERVER]: Error al añadir usuario: Usuario existente.\n");
    cJSON* errorJSON = cJSON_CreateObject();
    if(errorJSON == NULL){
        cJSON_Delete(errorJSON);
        return false;
    }
    cJSON_AddStringToObject(errorJSON, "type", "RESPONSE");
    cJSON_AddStringToObject(errorJSON, "operation", "IDENTIFY");
    cJSON_AddStringToObject(errorJSON, "result", "USER_ALREADY_EXISTS");
    cJSON_AddStringToObject(errorJSON, "extra", username);
    char* printedJSON = cJSON_PrintUnformatted(errorJSON);
    cJSON_Delete(errorJSON);
    sendMessage(printedJSON, clientFD);
    pthread_mutex_unlock(&userList->mutexLock);
    return false;
  }
  hashmap_set(userList->userList, &entry);
  pthread_mutex_unlock(&userList->mutexLock);
  return true;
}

/* Busca un nombre de usuario de una lista de usuarios dada y almacena su resultado en *result. Regresa false si ocurre algún error. */
bool GetUser(UserList* userList, const char* username,UserEntry* result){
  if(userList == NULL || userList->userList == NULL || username == NULL)
    return false;
  UserEntry entry = {0};
  strcpy(entry.username, username);
  pthread_mutex_lock(&userList->mutexLock);
  const UserEntry* found = hashmap_get(userList->userList, &entry);
  if(found!=NULL)
    *result = *found;
  pthread_mutex_unlock(&userList->mutexLock);
  return found != NULL;
}

bool DeleteUser(UserList* userList, const char* username) {
    if(userList == NULL || userList->userList == NULL || username == NULL) {
        printf("[SERVER]: Error. Lista de usuarios nula o usuario nulo.\n");
        return false;
    }
  UserEntry result = {0};
  bool deleted = false;
  if(GetUser(userList, username, &result)) {
    pthread_mutex_lock(&userList->mutexLock);
    const UserEntry* removed = hashmap_delete(userList->userList, &result);
    deleted = removed != NULL;
    pthread_mutex_unlock(&userList->mutexLock);
  }
  return deleted;
}

bool UserListIsEmpty(UserList* list) {
  return hashmap_count(list->userList) == 0;
}

bool UserToJSONIterator(const void* item, void* udata) {
  const UserEntry* ue = item;
  cJSON* json = udata;
  char* status;
  switch (ue->user.status) {
  case AWAY:
    status = "AWAY";
    break;
  case BUSY:
    status = "BUSY";
    break;
  case ACTIVE:
    status = "ACTIVE";
    break;
  }
  if(cJSON_AddStringToObject(json, ue->username, status) == NULL)
      return false;
  return true;
}

bool MessageSenderIterator(const void* item, void* udata) {
  const UserEntry* ue = item;
  Message* message = udata;
  if(!(message->clientFDSource == ue->user.clientFD))
      sendMessage(message->message, ue->user.clientFD);
  return true;
}

char* GenerateUserListJSON(UserList* list) {
  cJSON* completeJSON = cJSON_CreateObject();
  cJSON* userJSON = cJSON_CreateObject();
  if(completeJSON == NULL || userJSON == NULL){
    cJSON_Delete(completeJSON);
    cJSON_Delete(userJSON);
    return NULL;
  }
  cJSON_AddStringToObject(completeJSON, "type", "USER_LIST");
  pthread_mutex_lock(&list->mutexLock);
  hashmap_scan(list->userList, UserToJSONIterator, userJSON);
  pthread_mutex_unlock(&list->mutexLock);
  cJSON_AddItemToObject(completeJSON, "users", userJSON);
  char* printedJSON = cJSON_PrintUnformatted(completeJSON);
  cJSON_Delete(completeJSON);
  return printedJSON;
}

bool AuthenticateUser(UserList* userList, int clientFD, cJSON* json) {
  if(userList == NULL || userList->userList == NULL)
    return false;
  char username[USERNAME_MAX];
  if(!parseJSONValue(json, "username", username, sizeof(username))){
    printf("[JSON]: Valor inválido en username.\n");
    return false;
  }
  if(AddUser(userList, username, ACTIVE, clientFD)) {
    printf("[SERVER]: Se añadio al usuario %s.\n",username);
    return true;
  }
  return false;
}

bool StartFirstTimeAuthentication(char* buffer, UserList* userList, int clientFD, UserEntry* authUser) {
  if(buffer == NULL || userList == NULL || userList->userList == NULL)
    return false;
  cJSON* json = cJSON_Parse(buffer);
  if(!validJSON(json))
    return false;
  char value[20] = {0};
  if(!parseJSONValue(json,"type",value,sizeof(value)))
    return false;
  if(strcmp(value, "IDENTIFY") == 0) {
    if(AuthenticateUser(userList, clientFD, json)) {
      cJSON* responseJSON = cJSON_CreateObject();
      if(responseJSON == NULL){
        cJSON_Delete(responseJSON);
        cJSON_Delete(json);
        return false;
      }
      cJSON_AddStringToObject(responseJSON, "type", "RESPONSE");
      cJSON_AddStringToObject(responseJSON, "operation", "IDENTIFY");
      cJSON_AddStringToObject(responseJSON, "result", "SUCCESS");
      char username[USERNAME_MAX];
      parseJSONValue(json, "username", username, sizeof(username));
      cJSON_AddStringToObject(responseJSON, "extra", username);
      char* responseJSONString = cJSON_PrintUnformatted(responseJSON);
      sendMessage(responseJSONString, clientFD);
      free(responseJSONString);
      if(!GetUser(userList, username, authUser)) {
          cJSON_Delete(responseJSON);
          cJSON_Delete(json);
          printf("[SERVER]: Usuario se autenticó, pero no se pudo obtener su registro en lista.\n");
          return false;
      }
      cJSON_Delete(responseJSON);
      cJSON* newUserResponseForOtherUsers = cJSON_CreateObject();
      if(newUserResponseForOtherUsers == NULL){
          cJSON_Delete(newUserResponseForOtherUsers);
          return false;
      }
      cJSON_AddStringToObject(newUserResponseForOtherUsers, "type", "NEW_USER");
      cJSON_AddStringToObject(newUserResponseForOtherUsers, "username", username);
      char* newUserResponseForOtherUsersString = cJSON_PrintUnformatted(newUserResponseForOtherUsers);
      Message messageForOtherUsers = {.clientFDSource = clientFD, .message = newUserResponseForOtherUsersString};
      free(newUserResponseForOtherUsersString);
      pthread_mutex_lock(&userList->mutexLock);
      hashmap_scan(userList->userList, MessageSenderIterator, &messageForOtherUsers);
      pthread_mutex_unlock(&userList->mutexLock);
      cJSON_Delete(newUserResponseForOtherUsers);
    } else{
      cJSON_Delete(json);
      return false;
    }
  } else {
      printf("[SERVER]: Usuario no autenticado quiere realizar operaciones.\n");
      cJSON_Delete(json);
      return false;
  }
  cJSON_Delete(json);
  return true;
}

bool SendPublicText(char* buffer, UserList* list, char* username, int clientFD) {
  if(buffer == NULL || list == NULL || list->userList == NULL)
    return false;
  cJSON* publicTextJSON = cJSON_CreateObject();
  if(publicTextJSON == NULL)
      return false;
  cJSON_AddStringToObject(publicTextJSON, "type", "PUBLIC_TEXT_FROM");
  cJSON_AddStringToObject(publicTextJSON, "username", username);
  cJSON_AddStringToObject(publicTextJSON, "text", buffer);
  Message messageForOtherUsers = {.clientFDSource = clientFD, .message = cJSON_PrintUnformatted(publicTextJSON)};
  pthread_mutex_lock(&list->mutexLock);
  hashmap_scan(list->userList, MessageSenderIterator, &messageForOtherUsers);
  pthread_mutex_unlock(&list->mutexLock);
  cJSON_Delete(publicTextJSON);
  return true;
}

bool SendPrivateText(char* message, UserList* list, char* usernameSrc, char* usernameDest) {
  if(message == NULL || list == NULL || list->userList == NULL)
    return false;
  cJSON* publicTextJSON = cJSON_CreateObject();
  if(publicTextJSON == NULL)
      return false;
  UserEntry userDest;
  if(!GetUser(list, usernameDest, &userDest)) {
      GetUser(list,usernameSrc,&userDest);
      printf("[SERVER]: Error al mandar mensaje privado: Usuario no existente.");
      cJSON_AddStringToObject(publicTextJSON, "type", "RESPONSE");
      cJSON_AddStringToObject(publicTextJSON, "operation", "TEXT");
      cJSON_AddStringToObject(publicTextJSON, "result", "NO_SUCH_USER");
      cJSON_AddStringToObject(publicTextJSON, "extra", usernameDest);
      char* finalMessage = cJSON_PrintUnformatted(publicTextJSON);
      sendMessage(finalMessage, userDest.user.clientFD);
      cJSON_Delete(publicTextJSON);
      return true;
  }
  cJSON_AddStringToObject(publicTextJSON, "type", "TEXT_FROM");
  cJSON_AddStringToObject(publicTextJSON, "username", usernameSrc);
  cJSON_AddStringToObject(publicTextJSON, "text", message);
  char* finalMessage = cJSON_PrintUnformatted(publicTextJSON);
  sendMessage(finalMessage, userDest.user.clientFD);
  cJSON_Delete(publicTextJSON);
  return true;
}

bool ChangeUserStatus(UserList* list, char* username, int status, int clientFD) {
    if(list == NULL || list->userList == NULL || username == NULL)
        return false;
    if(strlen(username)>=USERNAME_MAX)
        return false;
    UserEntry entry = {0};
    if(!GetUser(list, username,&entry)) {
        printf("[SERVER]: Error: No se encontró al usuario al que se le quería cambiar el estado.\n");
        return false;
    }
    entry.user.status = status;
    pthread_mutex_lock(&list->mutexLock);
    hashmap_set(list->userList, &entry);
    cJSON* statusJSON = cJSON_CreateObject();
    if(statusJSON == NULL)
        return false;
    cJSON_AddStringToObject(statusJSON, "type", "NEW_STATUS");
    cJSON_AddStringToObject(statusJSON, "username", username);
    char* statusString;
    switch (status) {
        case AWAY:
            statusString = "AWAY";
            break;
        case BUSY:
            statusString = "BUSY";
            break;
        case ACTIVE:
            statusString = "ACTIVE";
            break;
    }
    cJSON_AddStringToObject(statusJSON, "status", statusString);
    Message messageForOtherUsers = {.clientFDSource = clientFD, .message = cJSON_PrintUnformatted(statusJSON)};
    hashmap_scan(list->userList, MessageSenderIterator, &messageForOtherUsers);
    pthread_mutex_unlock(&list->mutexLock);
    cJSON_Delete(statusJSON);
    return true;
}

int StatusStringToStatusInt(char* status) {
    if(strcmp(status, "AWAY") == 0) {
        return AWAY;
    } else if(strcmp(status, "ACTIVE") == 0) {
        return ACTIVE;
    } else if(strcmp(status, "BUSY") == 0) {
        return BUSY;
    } else
        return -1;
}

bool DisconnectUser(UserList* list, char* username, int clientFD) {
    if(!DeleteUser(list, username)) {
        printf("[SERVER]: Error de eliminación de usuario por desconexión.\n"); 
        return false;
    }
    close(clientFD);
    printf("[SERVER]: Se desconecto el usuario %s.\n",username);
    return true;
}

bool determineJSONResponse(char* buffer, UserList* list, int clientFD, char* usernameSrc) {
  if(buffer == NULL || list == NULL)
    return false;
  cJSON* json = cJSON_Parse(buffer);
  if(!validJSON(json))
    return false;
  char value[20] = {0};
  if(!parseJSONValue(json,"type",value,sizeof(value)))
    return false;
  if(strcmp(value, "IDENTIFY") == 0) {
      printf("[SERVER]: Usuario ya autenticado quiere autenticarse de nuevo. \n");
      return false;
  } else if (strcmp(value, "STATUS") == 0) {
      char statusMessage[10];
      if(!parseJSONValue(json,"status",statusMessage,sizeof(statusMessage)))
          return false;
      int status = StatusStringToStatusInt(statusMessage);
      if(status == -1)
          return false;
      ChangeUserStatus(list, usernameSrc, status, clientFD);
  } else if (strcmp(value, "USERS") == 0) {
      char* userListMessage = GenerateUserListJSON(list);
      if(userListMessage == NULL)
          return false;
      sendMessage(userListMessage,clientFD);
  } else if (strcmp(value, "TEXT") == 0) {
      char message[1024];
      char usernameDest[USERNAME_MAX];
      if(!parseJSONValue(json,"text",message,sizeof(message)) || !parseJSONValue(json,"username",usernameDest,sizeof(usernameDest)))
          return false;
      SendPrivateText(message, list, usernameSrc, usernameDest);
  } else if (strcmp(value, "PUBLIC_TEXT") == 0) {
      char message[1024];
      if(!parseJSONValue(json,"text",message,sizeof(message)))
          return false;
      SendPublicText(message, list,usernameSrc,clientFD);
  } else if (strcmp(value, "NEW_ROOM") == 0) {
    
  } else if (strcmp(value, "INVITE") == 0) {
    
  } else if (strcmp(value, "JOIN_ROOM") == 0) {
    
  } else if (strcmp(value, "ROOM_USERS") == 0) {
    
  } else if (strcmp(value, "ROOM_TEXT") == 0) {
    
  } else if (strcmp(value, "LEAVE_ROOM") == 0) {
    
  } else if (strcmp(value, "DISCONNECT") == 0) {
      return DisconnectUser(list, usernameSrc, clientFD);
  } else {
    return false;
  }
  cJSON_Delete(json);
  return true;
}
