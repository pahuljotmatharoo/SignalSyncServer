#ifndef THREAD_FUNCTIONS_H
#define THREAD_FUNCTIONS_H

#include "user.h"
#include "user_list.h"
#include "chat_room_list.h"
#include "messages.h"

typedef struct thread_arg {
    user*      curr;
    user_list* list_of_users;
    ChatRoomList* ChatRoom_list;
    user_map* user_Map;
    pthread_mutex_t* mutex;
    pthread_mutex_t* user_fileMutex;
    pthread_mutex_t* group_fileMutex;
} thread_arg;

size_t recv_exact_msg(void* buf, size_t len, int sock);
void recv_exact_username(char* temp, size_t len, int sock);
void recv_exact_png(char* temp, uint32_t len, int sock);
void *create_connection(void *arg);
void sendList(user_map* t_map);
void sendAllGroupMessages(user *new_user);
char** parseGroupString(char message[username_length + message_length]);
void send_chatroom_list(ChatRoomList* chatroom_list, int sockid);
void room_method_creation(user* temp, thread_arg* curr_user, int type_of_message, void* data, int size);
void room_method_message(recieved_message* a, user* temp, thread_arg* curr_user, int type_of_message, void* data, int size, thread_arg* threadArg);
void write_to_file_user(message_s * message_to_send_user, char* threadUsername, char* recvUsername, pthread_mutex_t *fileMutex);
void write_to_file_group(message_s_group * message_to_send_group, char* username, pthread_mutex_t *group_fileMutex);
void send_message_user(message_s *message_to_send, int current_user_socket, thread_arg* threadArg);
void setupDir(char* username);
char* setupFileStringUser(char *username, char* username_to_send);
char* setupFileStringGroup(char* group);
void sendUserRemoval(thread_arg* threadArg);
void sendPng(recieved_png* msg, thread_arg* threadArg);
void send_file(thread_arg* arg);
void process_file(recieved_png* png, uint32_t png_size);
uint32_t recv_file_size(thread_arg* arg);
void init_file_data_structure(recieved_png* png, uint32_t png_size);


#endif /* THREAD_FUNCTIONS_H */
