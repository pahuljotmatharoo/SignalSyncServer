#ifndef USER_LIST_H
#define USER_LIST_H
#include "user.h"
#include <pthread.h>
#define MAXUSERS 10
#define MAXGROUPS 10

typedef struct user_list {
    user *head;
    user *tail;
    unsigned int   size;
} user_list;

typedef struct {
    user*        m_userArr[MAXUSERS];
    unsigned int m_size;
} user_map;

void initUserMap(user_map* t_map);
void destroyUserMap(user_map* t_map);
size_t hash(char* username);
void insertUser(user_map* t_map, user* client);
void removeUser(user_map* t_map, user* client);
size_t findUser(user_map* t_map, char* username);

void init_user_list(user_list *a);
void destructor_user_list(user_list *ulist);
void insert_user(user_list *a, user *client);
void remove_user(user_list *a, user *client);
void print_client_list(user_list *a);

#endif /* USER_LIST_H */
