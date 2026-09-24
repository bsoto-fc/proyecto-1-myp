#ifndef ROOMS_H_
#define ROOMS_H_

#include <pthread.h>
#include <cjson/cJSON.h>
#include "users.h"
#include "hashmap.h"

bool CreateRoom(UserList* list, RoomsList* roomsList, char* roomname, char* username); 

bool InitRoomList(RoomsList* roomsList); 

bool AddRoom(UserList* list, RoomsList* roomsList, char* roomname, char* username,int srcClientFD);

void DestroyRoomList(RoomsList* roomsList);

bool GetRoom(RoomsList* roomsList, const char* roomname, RoomEntry* result); 

bool InviteToRoom(UserList* globalUsers, RoomsList* globalRooms, cJSON* usernames, char* roomname, char* usernameSrc, int clientFD); 

bool JoinRoom(RoomsList* globalRooms, char* roomname, char* usernameSrc, int clientFD); 

bool GetRoomUsers(RoomsList* globalRooms, char* roomname, char* usernameSrc, int clientFD); 

bool SendRoomText(char* buffer, RoomsList* globalRooms, char* roomname, char* username, int clientFD); 

bool LeaveRoom(RoomsList* globalRooms, char* roomname, char* usernameSrc, int clientFD); 

bool DeleteRoomUserIterator(const void* item, void* udata);

typedef struct {
    char* username;
    RoomsList* globalList;
} RoomDeleterAdapter;

#endif // ROOMS_H_
