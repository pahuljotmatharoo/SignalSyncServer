#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h> 
#include <string.h>
#include "thread_functions.h"
#include "user_list.h"
#include "messages.h"
#include "chat_room_list.h"
#define MSG_SEND 1
#define MSG_LIST 2
#define MSG_EXIT 3
#define username_length 50

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

int main() {
    atexit(cleanup);
    pthread_mutex_init(&mutex, NULL);
    pthread_mutex_init(&user_fileMutex, NULL);
    pthread_mutex_init(&group_fileMutex, NULL);

    user_Map = malloc(sizeof(user_map));

    initUserMap(user_Map);

    //create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);

    //create our server's information
    struct sockaddr_in server;
    server.sin_family = AF_INET; //specify to use IPV4
    server.sin_port = htons(2520); // port is 2520
    server.sin_addr.s_addr = htonl(INADDR_ANY); // this is our ipv4 address

    //bind socket to the port and server info
    bind(sock, (struct sockaddr*) &server, sizeof(server));

    //waiting for a connection
    listen(sock, 25);

    //we're gonna need our client list
    client_list = malloc(sizeof(user_list));
    init_user_list(client_list);

    //we're gonna need our chatroom list
    ChatRoom_list = malloc(sizeof(ChatRoomList));
    init_ChatRoom_list(ChatRoom_list);

    while (1) {
        //this is storing the clients information
        struct sockaddr_in client;
        socklen_t client_len = sizeof(client);
        int new_sock = accept(sock, (struct sockaddr*)&client, &client_len);

        if (new_sock < 0) {
            perror("accept");
            continue;
        }

        //set up the new user (should get cleaned by the destructor)
        user *new_user = malloc(sizeof(user));
        pthread_t id;
        new_user->client = client;
        new_user->next = NULL;
        new_user->sockid = new_sock;
        new_user -> id = id;

        recv_exact_username(new_user->username, username_length, new_user->sockid);

        //this is the thread argument, need the current user as well as the list
        thread_arg *arg = malloc(sizeof(thread_arg));
        arg->curr = new_user;
        arg->list_of_users = client_list;
        arg->ChatRoom_list = ChatRoom_list;
        arg->mutex = &mutex;
        arg->user_fileMutex = &user_fileMutex;
        arg->group_fileMutex = &group_fileMutex;
        arg->user_Map = user_Map;
        
        pthread_mutex_lock(&mutex);

        insertUser(user_Map, new_user);

        sendAllGroupMessages(new_user);

        pthread_create(&new_user->id, NULL, create_connection, arg);

        pthread_detach(new_user->id);

        sendList(user_Map);

        send_chatroom_list(ChatRoom_list, new_sock); // send list of groups only to new user

        pthread_mutex_unlock(&mutex);
}
}