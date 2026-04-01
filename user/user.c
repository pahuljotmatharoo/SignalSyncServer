#define USERNAME_LENGTH 50
#include <stdlib.h>
#include "user.h"
#include "../thread/thread_functions.h"

user* initUser(struct sockaddr_in* client, int sockid) {
    user *new_user = (user*)malloc(sizeof(user));

    new_user->client = *client;
    new_user->next = NULL;
    new_user->sockid = sockid;
    new_user->user_mutex = malloc(sizeof(pthread_mutex_t));
    new_user->username = malloc(USERNAME_LENGTH); // temporary fix
    uint32_t size = recvExactUsername(new_user->username, new_user->sockid);
    new_user->username_length = size;

    return new_user;
}