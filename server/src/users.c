#include "users.h"
#include "../util/socketutil.h"
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
  UserEntry* existingUser = hashmap_get(userList->userList,&entry);
  if(existingUser != NULL) {
    printf("[SERVER]: Error al autenticar usuario: Usuario existente.\n");
    pthread_mutex_unlock(&userList->mutexLock);
    return false;
  }
  hashmap_set(userList->userList, &entry);
  pthread_mutex_unlock(&userList->mutexLock);
  return true;
}

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
  if(userList == NULL || userList->userList == NULL || username == NULL)
    return false;
  UserEntry result = {0};
  bool deleted = false;
  if(GetUser(userList, username, &result) != false) {
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
  char* printedJSON = cJSON_Print(completeJSON);
  cJSON_Delete(completeJSON);
  return printedJSON;
}

bool AuthenticateUser(UserList* userList, int clientFD, cJSON* json) {
  char username[USERNAME_MAX];
  bool returnValue = false;
  if(!parseJSONValue(json, "username", username, sizeof(username))){
    printf("[JSON]: Valor inválido en username.\n");
  }
  if(AddUser(userList, username, ACTIVE, clientFD)) {
    printf("[SERVER]: Se añadio al usuario %s.\n",username);
    returnValue = true;
  }
  return returnValue;
}

bool determineJSONResponse(char* buffer, UserList* list, int clientFD) {
  if(buffer == NULL || list == NULL)
    return false;
  cJSON* json = cJSON_Parse(buffer);
  if(!validJSON(json))
    return false;
  char value[20] = {0};
  if(!parseJSONValue(json,"type",value,sizeof(value)))
    return false;
  else if(strcmp(value, "IDENTIFY") == 0) {
    return AuthenticateUser(list, clientFD, json);
  } else if (strcmp(value, "STATUS") == 0) {
    
  } else if (strcmp(value, "USERS") == 0) {
    
  } else if (strcmp(value, "TEXT") == 0) {
    
  } else if (strcmp(value, "PUBLIC_TEXT") == 0) {
    
  } else if (strcmp(value, "NEW_ROOM") == 0) {
    
  } else if (strcmp(value, "INVITE") == 0) {
    
  } else if (strcmp(value, "JOIN_ROOM") == 0) {
    
  } else if (strcmp(value, "ROOM_USERS") == 0) {
    
  } else if (strcmp(value, "ROOM_TEXT") == 0) {
    
  } else if (strcmp(value, "LEAVE_ROOM") == 0) {
    
  } else if (strcmp(value, "DISCONNECT") == 0) {
    
  } else {
    return false;
  }
  cJSON_Delete(json);
  return true;
}
