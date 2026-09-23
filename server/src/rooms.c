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

bool AddRoom(UserList* list, RoomsList* roomsList, char* roomname, char* username, int srcClientFD) {
    if(list == NULL || list->userList == NULL || roomsList == NULL || roomsList->roomsList == NULL || username == NULL)
        return false;
    if(strlen(roomname) >= ROOMNAME_MAX)
        return false;
    RoomEntry newRoom = { .roomUsers=list };
    strcpy(newRoom.roomname, roomname);
    pthread_mutex_lock(&roomsList->mutexLock);
    const RoomEntry* existingRoom = hashmap_get(roomsList->roomsList,&newRoom);
    if(existingRoom != NULL) {
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
        pthread_mutex_unlock(&roomsList->mutexLock);
        return false;
    }
    // TO DO: Hacer el siguiente código más seguro.
    UserEntry* userEntry = NULL;
    GetUser(list, username, userEntry);
    UserList* roomList = malloc(sizeof(UserList));
    InitUserList(roomList);
    AddUser(roomList, username, userEntry->user->status, srcClientFD);
    newRoom.roomUsers = roomList;
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

bool GetRoom(RoomsList* roomsList, char* roomname, RoomEntry* result) {
    if(roomsList == NULL || roomsList->roomsList == NULL || roomname == NULL)
        return false;
    RoomEntry entry = {0};
    strcpy(entry.roomname, roomname);
    pthread_mutex_lock(&roomsList->mutexLock);
    const RoomEntry* found = hashmap_get(roomsList->roomsList, &entry);
    if(found!=NULL)
        *result = *found;
    pthread_mutex_unlock(&roomsList->mutexLock);
    return found != NULL;
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
  UserEntry* foundUser = NULL;
  if(GetUser(message->invitedList, message->usernameSrc, foundUser) || GetUser(message->inRoomList,message->usernameSrc,foundUser))
      sendMessage(message->message, ue->user->clientFD);
  return true;
}

bool InviteToRoom(UserList* globalUsers, RoomsList* globalRooms, cJSON* usernames, char* roomname, char* usernameSrc, int clientFD) {
    // 1. Buscar que la sala exista.
    // 2. Buscar que todos los usuarios existan.
    RoomEntry* foundRoom = NULL;
    if(!GetRoom(globalRooms, roomname, foundRoom)) {
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
    UserEntry* foundUser = NULL;
    if(!GetUser(foundRoom->roomUsers, usernameSrc, foundUser)) {
        printf("[SERVER]: Usuario que no está en sala quiere invitar a más usuarios.\n");
        return false;
    }
    // Lista de invitaciones puede ser no vacía.
    UserList* roomUserInviteList = foundRoom->roomUsers;
    bool createdUserList = false;
    if(roomUserInviteList == NULL) {
        UserList* roomUserInviteList = malloc(sizeof(UserList));
        if(roomUserInviteList == NULL) {
            printf("[SERVER]: Error en reservación de memoria para lista de usuarios en sala\n");
            return false;
        }
        if(!InitUserList(roomUserInviteList)){
            printf("[SERVER]: Error en inicialización de lista de usuarios en sala.\n");
            free(roomUserInviteList);
            return false;
        }
        createdUserList = true;
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
                    DestroyUserList(roomUserInviteList);
                    free(roomUserInviteList);
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
                DestroyUserList(roomUserInviteList);
                free(roomUserInviteList);
                return true;
            } else {
                AddUser(roomUserInviteList, foundUser.username, foundUser.user->status, foundUser.user->clientFD);
            }
        } else {
            printf("[SERVER]: usernames en JSON recibido contiene un objeto que no es String.\n");
            DestroyUserList(roomUserInviteList);
            free(roomUserInviteList);
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
    InviteMessage messageForOtherUsers = {.clientFDSource = clientFD, .message = publicTextJSONString, .invitedList = roomUserInviteList, .inRoomList = foundRoom->roomUsers, .usernameSrc = usernameSrc};
    pthread_mutex_lock(&roomUserInviteList->mutexLock);
    hashmap_scan(roomUserInviteList->userList, InviteMessageSenderIterator, &messageForOtherUsers);
    if(createdUserList)
        foundRoom->invitedUsers = roomUserInviteList; 
    pthread_mutex_unlock(&roomUserInviteList->mutexLock);
    free(publicTextJSONString);
    cJSON_Delete(publicTextJSON);
    return true;
}

bool JoinRoom(RoomsList* globalRooms, char* roomname, char* usernameSrc, int clientFD) {
    RoomEntry* foundRoom = NULL;
    if(!GetRoom(globalRooms, roomname, foundRoom)) {
        printf("[SERVER]: No se encotró la habitación %s.\n",roomname);
        return false;
    }
    UserList* invitedUsers = foundRoom->invitedUsers;
    UserEntry* foundUser = NULL;
    if(!GetUser(invitedUsers, usernameSrc, foundUser)) {
        printf("[SERVER]: Usuario no fue invitado a la habitación %s y desea unirse.\n",roomname);
        return false;
    }
    if(!AddUser(foundRoom->roomUsers, foundUser->username, foundUser->user->status, foundUser->user->clientFD)) {
        printf("[SERVER]: Error aceptando invitación. \n");
        return false;
    }
}
