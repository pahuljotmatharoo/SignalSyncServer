#ifndef MESSAGES_H
#define MESSAGES_H
#include <stdint.h>
#define username_length 50
#define message_length 128
#define max_users 10

//message to send to the user
typedef struct message_to_send {
    char arr[message_length];
    char username[username_length];
} message_s;

//message to send to the user (group)
typedef struct message_to_send_group {
    char arr[message_length];
    char username[username_length];
    char groupName[username_length];
} message_s_group;

typedef struct message_recieved {
    char arr[message_length];
    char user_to_send[username_length];
    uint32_t size_m;
    uint32_t size_u;
} recieved_message;

typedef struct {
    char* arr;
    char user_to_send[username_length];
    char filename_to_send[username_length];
    uint32_t size_m;
    uint32_t size_u;
} recieved_png;

typedef struct list {
    uint32_t size;
    char arr[max_users][username_length];
} client_list_s;

typedef struct Msg{
    uint32_t type;    // message type
    uint32_t length;  // payload length
} MsgHeader;

void print_data(message_s *a);

#endif