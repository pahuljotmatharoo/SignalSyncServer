#ifndef THREAD_FUNCTIONS_H
#define THREAD_FUNCTIONS_H

#include <stdio.h>
#include "../user/user.h"
#include "../user/user_list.h"
#include "../chatroom/chat_room_list.h"
#include "../messages/messages.h"

typedef struct thread_arg {
    user*            curr;
    user_list*       list_of_users;
    ChatRoomList*    ChatRoom_list;
    user_map*        user_Map;
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
void writeToFileGroup(recieved_message* message_to_send_group, char* group, char* username, pthread_mutex_t* group_fileMutex);
void sendMessageUser(int current_user_socket, thread_arg* threadArg);
void setupDir(char* username);
char* setupFileStringUser(char *username, char* username_to_send);
char* setupFileStringGroup(char* group);
void sendUserRemoval(thread_arg* threadArg);
void sendFile(recievedFile* file, thread_arg* threadArg);
void sendFileUser(thread_arg* arg);
void processFile(recievedFile* file, uint32_t file_size);
uint32_t recvSize(int sockid);
void initFileDataStructure(recievedFile* file, uint32_t file_size);
void sendMessage(recieved_message* message_struct, int socket_id, int type_of_message);
void saveFile(recievedFile* file);
void downloadFile(thread_arg* threadArg);
int getFileSize(FILE* fp);
void sendFileGroup(thread_arg* arg);
void sendFileGroupMethod(recievedFile* file, thread_arg* threadArg);
void freeFile(recievedFile* file);
void freeRecievedMessage(recieved_message* recievedMessage);

#endif /* THREAD_FUNCTIONS_H */
