#ifndef USER_H
#define USER_H
#include <pthread.h>
#include <netinet/in.h>   /* for struct sockaddr_in */

typedef struct user {
    pthread_t         id;
    pthread_mutex_t*  user_mutex;
    struct sockaddr_in client;
    struct user      *next;
    int               sockid;
    char*             username;
    uint32_t          username_length;
} user;

#endif /* USER_H */
