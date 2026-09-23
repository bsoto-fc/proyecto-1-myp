#ifndef ROOMS_H_
#define ROOMS_H_

#include "hashmap.h"
#include <pthread.h>
#include "users.h"

bool CreateRoom(UserList* list, RoomsList* roomsList, char* roomname, char* username); 

bool InitRoomList(RoomsList* roomsList); 

bool AddRoom(UserList* list, RoomsList* roomsList, char* roomname, char* username,int srcClientFD);

void DestroyRoomList(RoomsList* roomsList);

bool GetRoom(RoomsList* roomsList, char* roomname, RoomEntry* result); 

#endif // ROOMS_H_
