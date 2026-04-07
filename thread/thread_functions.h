#ifndef THREAD_FUNCTIONS_H
#define THREAD_FUNCTIONS_H

#include <stdio.h>
#include "../user/user.h"
#include "../user/user_list.h"
#include "../chatroom/chat_room_list.h"
#include "../messages/messages.h"

typedef struct thread_arg {
    user*      curr;
    user_list* list_of_users;
    ChatRoomList* ChatRoom_list;
    user_map* user_Map;
    pthread_mutex_t* mutex;
    pthread_mutex_t* user_fileMutex;
    pthread_mutex_t* group_fileMutex;
} thread_arg;

char* recvExactMsg(uint32_t* len, int sock);
void *createConnection(void *arg);
void sendList(user_map* t_map, int sockid, pthread_mutex_t* socket_mutex);
void sendAllGroupMessages(user *new_user);
void sendUserJoin(user_map* t_map, user* new_user);
char** parseGroupString(char message[USERNAME_LENGTH + message_length]);
void sendChatroomList(ChatRoomList* chatroom_list, int sockid, pthread_mutex_t* socket_mutex);
void roomMethodCreation(thread_arg* curr_user, int type_of_message, void* data, int size);
void roomMethodMessage(thread_arg* curr_user);
void writeToFileUser(recieved_message* message_to_send_user, char* threadUsername, char* recvUsername, pthread_mutex_t *fileMutex);
void writeToFileGroup(recieved_message* message_to_send_group, char* username, pthread_mutex_t* group_fileMutex);
void sendMessageUser(int current_user_socket, thread_arg* threadArg);
void setupDir(char* username);
char* setupFileStringUser(char *username, char* username_to_send);
char* setupFileStringGroup(char* group);
void sendUserRemoval(thread_arg* threadArg);
void sendPng(recieved_png* msg, thread_arg* threadArg);
void sendFileUser(thread_arg* arg);
void processFile(recieved_png* png, uint32_t png_size);
uint32_t recvSize(int sockid);
void initFileDataStructure(recieved_png* png, uint32_t png_size);
void sendMessage(recieved_message* message_struct, int socket_id, int type_of_message);
void saveFile(recieved_png* png);
void downloadFile(thread_arg* threadArg);
int getFileSize(FILE* fp);

#endif /* THREAD_FUNCTIONS_H */
