#ifndef MESSAGES_H
#define MESSAGES_H

#include <stdint.h>
#define USERNAME_LENGTH 50
#define message_length 128
#define max_users 10

typedef struct message_recieved {
    char* arr;
    char* user_to_send;
    uint32_t size_m;
    uint32_t size_u;
} recieved_message;

typedef struct {
    char* arr;
    char* user_to_send;
    char* filename_to_send;
    uint32_t size_m;
    uint32_t size_u;
    uint32_t size_f_name;
} recievedFile;

typedef struct {
    char* filename;
    char* name;
    uint32_t filename_size;
    uint32_t name_size;
} recieved_file_info;

typedef struct list {
    uint32_t size;
    char arr[max_users][USERNAME_LENGTH];
} client_list_s;
#endif