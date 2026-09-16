#ifndef USERS_H_
#define USERS_H_

#include <pthread.h>
#include <stddef.h>
#include "hashmap.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define AWAY 1
#define ACTIVE 2
#define BUSY 3

#define USERNAME_MAX 64

typedef struct {
    int status;
    int clientFD;
} User;

typedef struct {
    char username[USERNAME_MAX];
    User user;
} UserEntry;

typedef struct {
    struct hashmap* userList;
    pthread_mutex_t mutexLock;
} UserList;

bool InitUserList(UserList* list);

void DestroyUserList(UserList* userList);

bool AddUser(UserList* userList, const char* username, int status, int clientFD); 

bool DeleteUser(UserList* userList, char* username);

bool GetUser(UserList* userList, const char* username,UserEntry* result);

#endif // USERS_H_
