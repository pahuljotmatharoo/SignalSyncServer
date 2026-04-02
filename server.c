#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h> 
#include <string.h>
#include "user/user.h"
#include "user/user_list.h"
#include "thread/thread_functions.h"
#include "messages/messages.h"
#include "chatroom/chat_room_list.h"
#define MSG_LIST 2
#define MSG_EXIT 3

//need to add a lock per socket, so we don't write to same socket at the same time

pthread_mutex_t mutex;
pthread_mutex_t user_fileMutex;
pthread_mutex_t group_fileMutex;
user_list *client_list;
ChatRoomList* ChatRoom_list;
user_map* user_Map;
int sock;

void cleanup() {
    destructor_user_list(client_list);
    destructor_ChatRoom_list(ChatRoom_list);
    destroyUserMap(user_Map);
    pthread_mutex_destroy(&mutex);
    pthread_mutex_destroy(&user_fileMutex);
    pthread_mutex_destroy(&group_fileMutex);
    close(sock);
}

// can't be bothered to put this in thread_functions.c, needs all the globals from server.c
thread_arg* setupThreadArg(user* new_user) {
    thread_arg *arg = malloc(sizeof(thread_arg));
    arg->curr = new_user;
    arg->list_of_users = client_list;
    arg->ChatRoom_list = ChatRoom_list;
    arg->mutex = &mutex;
    arg->user_fileMutex = &user_fileMutex;
    arg->group_fileMutex = &group_fileMutex;
    arg->user_Map = user_Map;
    return arg;
}

void main_function() {
    pthread_mutex_init(&mutex, NULL);
    pthread_mutex_init(&user_fileMutex, NULL);
    pthread_mutex_init(&group_fileMutex, NULL);

    user_Map = initUserMap();
    client_list = init_user_list();
    ChatRoom_list = init_ChatRoom_list();

    while (1) {
        struct sockaddr_in client;
        socklen_t client_len = sizeof(client);
        int new_sock = accept(sock, (struct sockaddr*)&client, &client_len);

        if (new_sock < 0) { continue; }

        user *new_user = initUser(&client, new_sock);

        thread_arg* arg = setupThreadArg(new_user);
        
        pthread_mutex_lock(&mutex);

        insertUser(user_Map, new_user);
        sendAllGroupMessages(new_user);
        pthread_create(&new_user->id, NULL, createConnection, arg);
        pthread_detach(new_user->id);
        sendList(user_Map, new_sock, new_user->user_mutex); // send list of all users only to new user
        sendChatroomList(ChatRoom_list, new_sock, new_user->user_mutex); // send list of groups only to new user
        sendUserJoin(user_Map, new_user); // send that THIS user joined to every connected user (except this new user)

        pthread_mutex_unlock(&mutex);
    }
}

int main() {
    atexit(cleanup);

    sock = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server;
    server.sin_family = AF_INET; //specify to use IPV4
    server.sin_port = htons(2520); // port is 2520
    server.sin_addr.s_addr = htonl(INADDR_ANY); // this is our ipv4 address

    bind(sock, (struct sockaddr*) &server, sizeof(server));

    listen(sock, 25);

    main_function();
}