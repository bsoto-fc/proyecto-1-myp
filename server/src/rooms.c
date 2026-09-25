#include <cjson/cJSON.h>
#include <pthread.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include "socketutil.h"
#include "rooms.h"


/* Boilerplate para comparar en diccionario. */
#pragma GCC diagnostic ignored "-Wunused-parameter" // Suprimir advertencia de parámetro sin usar. hashmap.c necesita una firma con void* udata.
int room_compare(const void *a, const void *b, void *udata) {
    const RoomEntry *ra = a;
    const RoomEntry *rb = b;
    return strcmp(ra->roomname, rb->roomname);
}
#pragma GCC diagnostic pop

/* Boilerplate de hash. */
uint64_t room_hash(const void *item, uint64_t seed0, uint64_t seed1) {
    const RoomEntry *room = item;
    return hashmap_sip(room->roomname, strlen(room->roomname), seed0, seed1);
}

void FreeRoomEntry(void* item) {
    RoomEntry* room = item;
    DestroyUserList(room->roomUsers);
    DestroyUserList(room->invitedUsers);
    free(room->roomUsers);
    free(room->invitedUsers);
    room->roomUsers = NULL;
    room->invitedUsers = NULL;
}

bool InitRoomList(RoomsList* roomsList) {
    roomsList->roomsList = hashmap_new(sizeof(RoomEntry), 0, 0, 0, room_hash, room_compare, FreeRoomEntry, NULL);
    if(roomsList->roomsList == NULL)
        return false;
    if(pthread_mutex_init(&roomsList->mutexLock, NULL) != 0){
        hashmap_free(roomsList->roomsList);
        roomsList->roomsList = NULL;
        return false;
    }
    return true;
}

void DestroyRoomList(RoomsList* roomsList){
    if(roomsList == NULL)
        return;
    pthread_mutex_destroy(&roomsList->mutexLock);
    hashmap_free(roomsList->roomsList);
    roomsList->roomsList = NULL;
} 

bool GetRoom(RoomsList* roomsList, const char* roomname, RoomEntry* result) {
    if(roomsList == NULL || roomsList->roomsList == NULL || roomname == NULL)
        return false;
    RoomEntry entry = {0};
    if(strlen(roomname)>=ROOMNAME_MAX)
        return false;
    strcpy(entry.roomname, roomname);
    pthread_mutex_lock(&roomsList->mutexLock);
    const RoomEntry* found = hashmap_get(roomsList->roomsList, &entry);
    if(found!=NULL)
        *result = *found;
    pthread_mutex_unlock(&roomsList->mutexLock);
    return found != NULL;
}

bool AddRoom(UserList* list, RoomsList* roomsList, char* roomname, char* username, int srcClientFD) {
    if(list == NULL || list->userList == NULL || roomsList == NULL || roomsList->roomsList == NULL || username == NULL)
        return false;
    if(strlen(roomname) >= ROOMNAME_MAX)
        return false;
    RoomEntry newRoom = {0};
    strcpy(newRoom.roomname, roomname);
    RoomEntry existingRoom = {0};
    if(GetRoom(roomsList, roomname, &existingRoom)) {
        printf("[SERVER]: Error al crear sala: Sala existente.\n");
        cJSON* errorJSON = cJSON_CreateObject();
        if(errorJSON == NULL){
            cJSON_Delete(errorJSON);
            return false;
        }
        cJSON_AddStringToObject(errorJSON, "type", "RESPONSE");
        cJSON_AddStringToObject(errorJSON, "operation", "NEW_ROOM");
        cJSON_AddStringToObject(errorJSON, "result", "ROOM_ALREADY_EXISTS");
        cJSON_AddStringToObject(errorJSON, "extra", roomname);
        char* printedJSON = cJSON_PrintUnformatted(errorJSON);
        cJSON_Delete(errorJSON);
        sendMessage(printedJSON, srcClientFD);
        free(printedJSON);
        return true;
    }
    pthread_mutex_lock(&roomsList->mutexLock);
    // TO DO: Hacer el siguiente código más seguro.
    UserEntry userEntry = {0};
    if(!GetUser(list, username, &userEntry)) {
        printf("[SERVER]: Error al obtener usuario.\n");
        pthread_mutex_unlock(&roomsList->mutexLock);
        return false;
    }
    UserList* roomList = malloc(sizeof(UserList));
    if(roomList == NULL || !InitUserListRef(roomList)){
        free(roomList);
        pthread_mutex_unlock(&roomsList->mutexLock);
        return false;
    }
    UserList* invitedList = malloc(sizeof(UserList));
    if(invitedList == NULL || !InitUserListRef(invitedList)) {
        DestroyUserList(roomList);
        free(roomList);
        free(invitedList);
        pthread_mutex_unlock(&roomsList->mutexLock);
        return false;
    }
    AddUserRef(roomList, username, userEntry.user);
    newRoom.roomUsers = roomList;
    newRoom.invitedUsers = invitedList;
    hashmap_set(roomsList->roomsList, &newRoom);
    cJSON* successJSON = cJSON_CreateObject();
    if(successJSON == NULL){
        cJSON_Delete(successJSON);
        return false;
    }
    cJSON_AddStringToObject(successJSON, "type", "RESPONSE");
    cJSON_AddStringToObject(successJSON, "operation", "NEW_ROOM");
    cJSON_AddStringToObject(successJSON, "result", "SUCCESS");
    cJSON_AddStringToObject(successJSON, "extra", roomname);
    char* printedJSON = cJSON_PrintUnformatted(successJSON);
    cJSON_Delete(successJSON);
    sendMessage(printedJSON, srcClientFD);
    free(printedJSON);
    pthread_mutex_unlock(&roomsList->mutexLock);
    return true;
}

bool DeleteRoom(RoomsList* roomsList, const char* roomname) {
    if(roomsList == NULL || roomsList->roomsList == NULL || roomname == NULL)
        return false;
    RoomEntry result = {0};
    bool deleted = false;
    if(GetRoom(roomsList, roomname, &result)) {
        pthread_mutex_lock(&roomsList->mutexLock);
        const RoomEntry* removed = hashmap_delete(roomsList->roomsList,&result);
        deleted = removed != NULL;
        pthread_mutex_unlock(&roomsList->mutexLock);
    }
    return deleted;
}
typedef struct {
    int clientFDSource;
    char* usernameSrc;
    UserList* invitedList;
    UserList* inRoomList;
    char* message;
} InviteMessage;

bool InviteMessageSenderIterator(const void* item, void* udata) {
  const UserEntry* ue = item;
  InviteMessage* message = udata;
  sendMessage(message->message, ue->user->clientFD);
  return true;
}

bool InviteToRoom(UserList* globalUsers, RoomsList* globalRooms, cJSON* usernames, char* roomname, char* usernameSrc, int clientFD) {
    // 1. Buscar que la sala exista.
    // 2. Buscar que todos los usuarios existan.
    RoomEntry foundRoom = {0};
    if(!GetRoom(globalRooms, roomname, &foundRoom)) {
        printf("[SERVER]: No se encotró la habitación %s.\n",roomname);
        cJSON* disconnectJSON = cJSON_CreateObject();
        if(disconnectJSON == NULL) {
            printf("[SERVER]: Error al mandar respuesta de habitación inexistente.\n");
            return false;
        }
        cJSON_AddStringToObject(disconnectJSON, "type", "RESPONSE");
        cJSON_AddStringToObject(disconnectJSON, "operation", "INVITE");
        cJSON_AddStringToObject(disconnectJSON, "result", "NO_SUCH_ROOM");
        cJSON_AddStringToObject(disconnectJSON, "extra", roomname);
        char* disconnectJSONString = cJSON_PrintUnformatted(disconnectJSON);
        sendMessage(disconnectJSONString, clientFD);
        free(disconnectJSONString);
        cJSON_Delete(disconnectJSON);
        return false;
    }
    // Checar si usuario pertenece a la sala.
    UserEntry foundUser = {0};
    if(!GetUser(foundRoom.roomUsers, usernameSrc, &foundUser)) {
        printf("[SERVER]: Usuario que no está en sala quiere invitar a más usuarios.\n");
        return false;
    }
    // Lista de invitaciones puede ser no vacía.
    UserList* roomUserInviteList = foundRoom.invitedUsers;
    if(roomUserInviteList == NULL){
        printf("[SERVER]: La sala %s no tiene lista de usuarios inicializada.\n",roomname);
        return false;
    }
    cJSON* username = NULL;
    cJSON_ArrayForEach(username, usernames) {
        if(cJSON_IsString(username)) {
            UserEntry foundUser = {0};
            if(!GetUser(globalUsers, username->valuestring, &foundUser)) {
                printf("[SERVER]: usernames en JSON recibido contiene un usuario no existente.\n");
                cJSON* disconnectJSON = cJSON_CreateObject();
                if(disconnectJSON == NULL) {
                    printf("[SERVER]: Error al mandar respuesta de usuario inexistente.\n");
                    return false;
                }
                cJSON_AddStringToObject(disconnectJSON, "type", "RESPONSE");
                cJSON_AddStringToObject(disconnectJSON, "operation", "INVITE");
                cJSON_AddStringToObject(disconnectJSON, "result", "NO_SUCH_USER");
                cJSON_AddStringToObject(disconnectJSON, "extra", username->valuestring);
                char* disconnectJSONString = cJSON_PrintUnformatted(disconnectJSON);
                sendMessage(disconnectJSONString, clientFD);
                free(disconnectJSONString);
                cJSON_Delete(disconnectJSON);
                return true;
            } else {
                AddUserRef(roomUserInviteList, foundUser.username, foundUser.user);
            }
        } else {
            printf("[SERVER]: usernames en JSON recibido contiene un objeto que no es String.\n");
            return false;
        }
    }
    cJSON* publicTextJSON = cJSON_CreateObject();
    if(publicTextJSON == NULL)
        return false;
    cJSON_AddStringToObject(publicTextJSON, "type", "INVITATION");
    cJSON_AddStringToObject(publicTextJSON, "username", usernameSrc);
    cJSON_AddStringToObject(publicTextJSON, "roomname", roomname);
    char* publicTextJSONString = cJSON_PrintUnformatted(publicTextJSON);
    InviteMessage messageForOtherUsers = {.clientFDSource = clientFD, .message = publicTextJSONString, .invitedList = roomUserInviteList, .inRoomList = foundRoom.roomUsers, .usernameSrc = usernameSrc};
    pthread_mutex_lock(&roomUserInviteList->mutexLock);
    hashmap_scan(roomUserInviteList->userList, InviteMessageSenderIterator, &messageForOtherUsers);
    pthread_mutex_unlock(&roomUserInviteList->mutexLock);
    free(publicTextJSONString);
    cJSON_Delete(publicTextJSON);
    return true;
}

bool SendRawText(char* buffer, UserList* list, int clientFD) {
  if(buffer == NULL || list == NULL || list->userList == NULL)
    return false;
  Message messageForOtherUsers = {.clientFDSource = clientFD, .message = buffer };
  pthread_mutex_lock(&list->mutexLock);
  hashmap_scan(list->userList, MessageSenderIterator, &messageForOtherUsers);
  pthread_mutex_unlock(&list->mutexLock);
  return true;
}

bool JoinRoom(RoomsList* globalRooms, char* roomname, char* usernameSrc, int clientFD) {
    RoomEntry foundRoom = {0};
    if(!GetRoom(globalRooms, roomname, &foundRoom)) {
        printf("[SERVER]: No se encontró la habitación %s.\n",roomname);
        cJSON* notFoundJSON = cJSON_CreateObject();
        if(notFoundJSON == NULL) {
            printf("[SERVER]: Error creando JSON de respuesta.\n");
            return false;
        }
        cJSON_AddStringToObject(notFoundJSON, "type", "INVITATION");
        cJSON_AddStringToObject(notFoundJSON, "operation", "JOIN_ROOM");
        cJSON_AddStringToObject(notFoundJSON, "result", "NO_SUCH_ROOM");
        cJSON_AddStringToObject(notFoundJSON, "extra", roomname);
        char* notFoundJSONString = cJSON_PrintUnformatted(notFoundJSON);
        sendMessage(notFoundJSONString, clientFD);
        free(notFoundJSONString);
        cJSON_Delete(notFoundJSON);
        return false;
    }
    UserList* invitedUsers = foundRoom.invitedUsers;
    UserEntry foundUser = {0};
    if(!GetUser(invitedUsers, usernameSrc, &foundUser)) {
        printf("[SERVER]: Usuario no fue invitado a la habitación %s y desea unirse.\n",roomname);
        cJSON* notInvitedJSON = cJSON_CreateObject();
        if(notInvitedJSON == NULL) {
            printf("[SERVER]: Error creando JSON de respuesta.\n");
            return false;
        }
        cJSON_AddStringToObject(notInvitedJSON, "type", "INVITATION");
        cJSON_AddStringToObject(notInvitedJSON, "operation", "JOIN_ROOM");
        cJSON_AddStringToObject(notInvitedJSON, "result", "NOT_INVITED");
        cJSON_AddStringToObject(notInvitedJSON, "extra", roomname);
        char* notInvitedJSONString = cJSON_PrintUnformatted(notInvitedJSON);
        sendMessage(notInvitedJSONString, clientFD);
        free(notInvitedJSONString);
        cJSON_Delete(notInvitedJSON);
        return false;
    }
    if(!AddUserRef(foundRoom.roomUsers, foundUser.username, foundUser.user)) {
        printf("[SERVER]: Error aceptando invitación. \n");
        return false;
    }

    if(!DeleteUser(invitedUsers, foundUser.username)) {
        printf("[SERVER]: Error sacando usuario de lista de invitados al unir usuario.\n");
        return false;
    }
    cJSON* successJSON = cJSON_CreateObject();
    if(successJSON == NULL) {
        printf("[SERVER]: Error creando JSON de respuesta.\n");
        return false;
    }
    cJSON_AddStringToObject(successJSON, "type", "RESPONSE");
    cJSON_AddStringToObject(successJSON, "operation", "JOIN_ROOM");
    cJSON_AddStringToObject(successJSON, "result", "SUCCESS");
    cJSON_AddStringToObject(successJSON, "extra", roomname);
    char* successJSONString = cJSON_PrintUnformatted(successJSON);
    sendMessage(successJSONString, clientFD);
    free(successJSONString);
    cJSON_Delete(successJSON);
    
    cJSON* publicTextJSON = cJSON_CreateObject();
    if(publicTextJSON == NULL)
        return false;
    cJSON_AddStringToObject(publicTextJSON, "type", "JOINED_ROOM");
    cJSON_AddStringToObject(publicTextJSON, "roomname", roomname);
    cJSON_AddStringToObject(publicTextJSON, "username", usernameSrc);
    char* publicTextJSONString = cJSON_PrintUnformatted(publicTextJSON);
    SendRawText(publicTextJSONString, foundRoom.roomUsers, clientFD);
    free(publicTextJSONString);
    cJSON_Delete(publicTextJSON);
    return true;
}

char* GenerateRoomUserListJSON(UserList* list,char* roomname) {
  cJSON* completeJSON = cJSON_CreateObject();
  cJSON* userJSON = cJSON_CreateObject();
  if(completeJSON == NULL || userJSON == NULL){
    cJSON_Delete(completeJSON);
    cJSON_Delete(userJSON);
    return NULL;
  }
  cJSON_AddStringToObject(completeJSON, "type", "ROOM_USER_LIST");
  cJSON_AddStringToObject(completeJSON, "roomname", roomname);
  pthread_mutex_lock(&list->mutexLock);
  hashmap_scan(list->userList, UserToJSONIterator, userJSON);
  pthread_mutex_unlock(&list->mutexLock);
  cJSON_AddItemToObject(completeJSON, "users", userJSON);
  char* printedJSON = cJSON_PrintUnformatted(completeJSON);
  cJSON_Delete(completeJSON);
  return printedJSON;
}

bool GetRoomUsers(RoomsList* globalRooms, char* roomname, char* usernameSrc, int clientFD) {
    RoomEntry foundRoom = {0};
    if(!GetRoom(globalRooms, roomname, &foundRoom)) {
        printf("[SERVER]: No se encontró la habitación solicitada.\n");
        cJSON* failJSON = cJSON_CreateObject();
        if(failJSON == NULL)
            return false;
        cJSON_AddStringToObject(failJSON, "type", "RESPONSE");
        cJSON_AddStringToObject(failJSON, "operation", "ROOM_USERS");
        cJSON_AddStringToObject(failJSON, "result", "NO_SUCH_ROOM");
        cJSON_AddStringToObject(failJSON, "extra", roomname);
        char* failJSONString = cJSON_PrintUnformatted(failJSON);
        sendMessage(failJSONString, clientFD);
        free(failJSONString);
        cJSON_Delete(failJSON);
        return true;
    }
    UserList* roomUsers = foundRoom.roomUsers;
    if(roomUsers == NULL) {
        printf("[SERVER]: No hay usuarios en habitación.\n");
    }
    UserEntry foundUser = {0};
    if(!GetUser(roomUsers, usernameSrc, &foundUser)) {
        printf("[SERVER]: Usuario no pertenece a habitación.\n");
        cJSON* failJSON = cJSON_CreateObject();
        if(failJSON == NULL)
            return false;
        cJSON_AddStringToObject(failJSON, "type", "RESPONSE");
        cJSON_AddStringToObject(failJSON, "operation", "ROOM_USERS");
        cJSON_AddStringToObject(failJSON, "result", "NOT_JOINED");
        cJSON_AddStringToObject(failJSON, "extra", roomname);
        char* failJSONString = cJSON_PrintUnformatted(failJSON);
        sendMessage(failJSONString, clientFD);
        free(failJSONString);
        cJSON_Delete(failJSON);
        return false;
    }
    char* roomUserListJSON = GenerateRoomUserListJSON(roomUsers, roomname);
    sendMessage(roomUserListJSON, clientFD);
    free(roomUserListJSON);
    return true;
} 

bool SendRoomText(char* buffer, RoomsList* globalRooms, char* roomname, char* username, int clientFD) {
    RoomEntry foundRoom = {0};
    if(!GetRoom(globalRooms, roomname, &foundRoom)) {
        printf("[SERVER]: No se encontró la habitación solicitada.\n");
        cJSON* failJSON = cJSON_CreateObject();
        if(failJSON == NULL)
            return false;
        cJSON_AddStringToObject(failJSON, "type", "RESPONSE");
        cJSON_AddStringToObject(failJSON, "operation", "ROOM_TEXT");
        cJSON_AddStringToObject(failJSON, "result", "NO_SUCH_ROOM");
        cJSON_AddStringToObject(failJSON, "extra", roomname);
        char* failJSONString = cJSON_PrintUnformatted(failJSON);
        sendMessage(failJSONString, clientFD);
        free(failJSONString);
        cJSON_Delete(failJSON);
        return false;
    }
    UserList* roomUsers = foundRoom.roomUsers;
    if(roomUsers == NULL) {
        printf("[SERVER]: No hay usuarios en habitación.\n");
    }
    UserEntry foundUser = {0};
    if(!GetUser(roomUsers, username, &foundUser)) {
        printf("[SERVER]: Usuario no pertenece a habitación.\n");
        cJSON* failJSON = cJSON_CreateObject();
        if(failJSON == NULL)
            return false;
        cJSON_AddStringToObject(failJSON, "type", "RESPONSE");
        cJSON_AddStringToObject(failJSON, "operation", "ROOM_TEXT");
        cJSON_AddStringToObject(failJSON, "result", "NOT_JOINED");
        cJSON_AddStringToObject(failJSON, "extra", roomname);
        char* failJSONString = cJSON_PrintUnformatted(failJSON);
        sendMessage(failJSONString, clientFD);
        free(failJSONString);
        cJSON_Delete(failJSON);
        return false;
    }
    cJSON* textJSON = cJSON_CreateObject();
    if(textJSON == NULL)
        return false;
    cJSON_AddStringToObject(textJSON, "type", "ROOM_TEXT_FROM");
    cJSON_AddStringToObject(textJSON, "roomname", roomname);
    cJSON_AddStringToObject(textJSON, "username", username);
    cJSON_AddStringToObject(textJSON, "text", buffer);
    char* textJSONString = cJSON_PrintUnformatted(textJSON);
    if(!SendRawText(textJSONString, roomUsers, clientFD)) {
        printf("[SERVER]: Error al mandar mensaje en habitación.\n");
        return false;
    }
    free(textJSONString);
    cJSON_Delete(textJSON);
    return true;
}

bool LeaveRoom(RoomsList* globalRooms, char* roomname, char* usernameSrc, int clientFD) {
    RoomEntry foundRoom = {0};
    if(!GetRoom(globalRooms, roomname, &foundRoom)) {
        printf("[SERVER]: No se encontró la habitación solicitada.\n");
        cJSON* failJSON = cJSON_CreateObject();
        if(failJSON == NULL)
            return false;
        cJSON_AddStringToObject(failJSON, "type", "RESPONSE");
        cJSON_AddStringToObject(failJSON, "operation", "LEAVE_ROOM");
        cJSON_AddStringToObject(failJSON, "result", "NO_SUCH_ROOM");
        cJSON_AddStringToObject(failJSON, "extra", roomname);
        char* failJSONString = cJSON_PrintUnformatted(failJSON);
        sendMessage(failJSONString, clientFD);
        free(failJSONString);
        cJSON_Delete(failJSON);
        return false;
    }
    UserList* roomUsers = foundRoom.roomUsers;
    if(roomUsers == NULL) {
        printf("[SERVER]: No hay usuarios en habitación.\n");
    }
    UserEntry foundUser = {0};
    if(!GetUser(roomUsers, usernameSrc, &foundUser)) {
        printf("[SERVER]: Usuario no pertenece a habitación.\n");
        cJSON* failJSON = cJSON_CreateObject();
        if(failJSON == NULL)
            return false;
        cJSON_AddStringToObject(failJSON, "type", "RESPONSE");
        cJSON_AddStringToObject(failJSON, "operation", "LEAVE_ROOM");
        cJSON_AddStringToObject(failJSON, "result", "NOT_JOINED");
        cJSON_AddStringToObject(failJSON, "extra", roomname);
        char* failJSONString = cJSON_PrintUnformatted(failJSON);
        sendMessage(failJSONString, clientFD);
        free(failJSONString);
        cJSON_Delete(failJSON);
        return false;
    }
    cJSON* leaveJSON = cJSON_CreateObject();
    if(leaveJSON == NULL)
        return false;
    cJSON_AddStringToObject(leaveJSON, "type", "LEFT_ROOM");
    cJSON_AddStringToObject(leaveJSON, "roomname", roomname);
    cJSON_AddStringToObject(leaveJSON, "username", usernameSrc);
    char* leaveJSONString = cJSON_PrintUnformatted(leaveJSON);
    if(!SendRawText(leaveJSONString, roomUsers, clientFD)) {
        printf("[SERVER]: Error al mandar mensaje en habitación.\n");
        return false;
    }
    free(leaveJSONString);
    cJSON_Delete(leaveJSON);
    if(!DeleteUser(roomUsers, usernameSrc)) {
        printf("[SERVER]: Error al desconectar usuario de habitación.\n");
    }
    if(UserListIsEmpty(roomUsers) && UserListIsEmpty(foundRoom.invitedUsers)) {
        if(!DeleteRoom(globalRooms, foundRoom.roomname)) {
            printf("[SERVER]: Sala sin usuarios no pudo ser eliminada.\n");
        }
    }

    return true;
}

bool DeleteRoomUserIterator(const void* item, void* udata) {
  const RoomEntry* re = item;
  const RoomDeleterAdapter* adapter = udata;
  UserEntry foundUser = {0};
  if(GetUser(re->roomUsers, adapter->username, &foundUser)) {
      if(!DeleteUser(re->roomUsers, adapter->username)) {
          printf("[SERVER]: Error eliminando usuario de sala %s por desconexión.\n",re->roomname);
      }
  }
  if(UserListIsEmpty(re->roomUsers) && UserListIsEmpty(re->invitedUsers)) {
      if(!DeleteRoom(adapter->globalList, re->roomname)) {
          printf("[SERVER]: Sala sin usuarios no pudo ser eliminada.\n");
      }
  }
      
  return true;
}
