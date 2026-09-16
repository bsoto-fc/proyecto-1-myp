#include "users.h"
#include "../util/socketutil.h"
#include <pthread.h>
#include <string.h>
#include <stdio.h>

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

bool SendMessageToUser(const void* item, void* udata){
  const UserEntry* ue = item;
  char* message = udata;
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
  UserEntry entry = { .user = {.status = status, .clientFD = clientFD} };
  strcpy(entry.username, username);
  pthread_mutex_lock(&userList->mutexLock);
  hashmap_set(userList->userList, &entry);
  pthread_mutex_unlock(&userList->mutexLock);
  return true;
}

bool GetUser(UserList* userList, const char* username,UserEntry* result){
  if(userList == NULL || username == NULL)
    return false;
  UserEntry entry = {0};
  strcpy(entry.username, username);
  pthread_mutex_lock(&userList->mutexLock);
  UserEntry* found = hashmap_get(userList->userList, &entry);
  if(found!=NULL)
    *result = *found;
  pthread_mutex_unlock(&userList->mutexLock);
  return found != NULL;
}

bool DeleteUser(UserList* userList, char* username) {
  UserEntry result = {0};
  bool deleted = false;
  if(GetUser(userList, username, &result) != false) {
    pthread_mutex_lock(&userList->mutexLock);
    UserEntry* removed = hashmap_delete(userList->userList, &result);
    deleted = removed != NULL;
    pthread_mutex_unlock(&userList->mutexLock);
  }
  return deleted;
}

