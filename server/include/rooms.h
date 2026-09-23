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

bool GetRoom(RoomsList* roomsList, char* roomname, RoomEntry* result); 

bool InviteToRoom(UserList* globalUsers, RoomsList* globalRooms, cJSON* usernames, char* roomname, char* usernameSrc, int clientFD); 

bool JoinRoom(RoomsList* globalRooms, char* roomname, char* usernameSrc, int clientFD); 

#endif // ROOMS_H_
