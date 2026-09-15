#ifndef USERS_H_
#define USERS_H_

#define AWAY 1
#define ACTIVE 2
#define BUSY 3

typedef struct {
  char* username;
  int status;
} User;

typedef struct {

} UserList;

#endif // USERS_H_
