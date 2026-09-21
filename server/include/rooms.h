#ifndef ROOMS_H_
#define ROOMS_H_

#include "hashmap.h"
#include "users.h"
#include <pthread.h>

typedef struct {
    int roomNo;
    UserList roomUsers;
    pthread_mutex_t mutexLock;
} Room;

#endif // ROOMS_H_
