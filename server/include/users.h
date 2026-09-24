#ifndef USERS_H_
#define USERS_H_

#include <pthread.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include "hashmap.h"
#include "jsonutil.h"

#define AWAY 1
#define ACTIVE 2
#define BUSY 3

#define USERNAME_MAX 9 // 8 caracteres + \0
#define ROOMNAME_MAX 17 // 16 caracteres + \0

typedef struct {
    uint8_t status;
    int clientFD;
} User;

typedef struct {
    char username[USERNAME_MAX];
    User* user;
} UserEntry;

typedef struct {
    struct hashmap* userList;
    pthread_mutex_t mutexLock;
} UserList;

typedef struct {
    int clientFDSource;
    char* message;
} Message;

typedef struct {
    char roomname[ROOMNAME_MAX]; 
    UserList* roomUsers;
    UserList* invitedUsers;
} RoomEntry;

typedef struct {
    struct hashmap* roomsList;
    pthread_mutex_t mutexLock;
} RoomsList;

bool InitUserList(UserList* list);

void DestroyUserList(UserList* userList);

bool AddUser(UserList* userList, const char* username, int status, int clientFD); 

bool DeleteUser(UserList* userList, const char* username); 

bool GetUser(UserList* userList, const char* username,UserEntry* result);

char* GenerateUserListJSON(UserList* list); 

bool determineJSONResponse(char* buffer, UserList* list, int clientFD, char* usernameSrc, RoomsList* roomsList); 

bool StartFirstTimeAuthentication(char* buffer, UserList* userList, int clientFD, UserEntry* authUser); 

bool DisconnectUser(UserList* list, RoomsList* roomsList, char* username, int clientFD); 

bool MessageSenderIterator(const void* item, void* udata); 

bool UserToJSONIterator(const void* item, void* udata); 

bool SendPublicText(char* buffer, UserList* list, char* username, int clientFD); 

bool InitUserListRef(UserList* list);

bool AddUserRef(UserList* userList, const char* username, User* user);

bool UserListIsEmpty(UserList* list); 

#endif // USERS_H_
