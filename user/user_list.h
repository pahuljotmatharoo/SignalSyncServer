#ifndef USER_LIST_H
#define USER_LIST_H
#include "user.h"
#include <pthread.h>
#define MAXUSERS 10
#define MAXGROUPS 10

typedef struct {
    user*       head;
    user*       tail;
    unsigned int size;
} user_list;

typedef struct {
    user*        m_userArr[MAXUSERS];
    unsigned int m_size;
} user_map;

typedef struct {
    char**        m_files[MAXUSERS];
    user*         m_users[MAXUSERS];
    unsigned int  m_size[MAXUSERS];
    unsigned int  m_capacity[MAXUSERS];
} user_files;

typedef struct {
    int              sockid;
    pthread_mutex_t* mutex;
} user_info;

user_map*  initUserMap();
user_files* initUserFilesMap();
void destroyUserMap(user_map* t_map);
void destroyUserFileMap(user_files* map);
size_t hash(char* username);
void insertUser(user_map* t_map, user_files* t_files, user* client);
void removeUser(user_map* t_map, user* client);
user_info findUser(user_map* t_map, char* username);
void insertFile(user_files* t_map, user* client, char* file);

user_list* init_user_list();
void destructor_user_list(user_list *ulist);
void insert_user(user_list *a, user *client);
void remove_user(user_list *a, user *client);
void print_client_list(user_list *a);

char** resizeArray(char** arr, unsigned int* size);

#endif
