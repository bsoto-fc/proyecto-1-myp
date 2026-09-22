#include <pthread.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include "socketutil.h"
#include "rooms.h"


/* Boilerplate para comparar en diccionario. */
int room_compare(const void *a, const void *b, void *udata) {
    const RoomEntry *ra = a;
    const RoomEntry *rb = b;
    return strcmp(ra->roomname, rb->roomname);
}

/* Boilerplate de hash. */
uint64_t room_hash(const void *item, uint64_t seed0, uint64_t seed1) {
    const RoomEntry *room = item;
    return hashmap_sip(room->roomname, strlen(room->roomname), seed0, seed1);
}

void FreeRoomEntry(void* item) {
    RoomEntry* room = item;
    DestroyUserList(room->roomUsers);
    room->roomUsers = NULL;
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
    AddUser(roomList, username, userEntry->user.status, srcClientFD);
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
