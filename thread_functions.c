#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "user_list.h"
#include <sys/stat.h>
#include <sys/types.h>
#include "thread_functions.h"
#include "messages.h"
#define MSG_SEND 1
#define MSG_LIST 2
#define MSG_EXIT 3
#define USER_EXIT 4
#define ROOM_CREATE 5
#define ROOM_MSG 6
#define ROOM_LIST 7
#define PNG_SEND 8
#define FILE_GROUP 9
#define username_length 50
#define message_length 128

char** parseGroupString(char message[username_length + message_length]) {
    char** return_strings = malloc(2);
    char* left = malloc(username_length);
    char* right = malloc(message_length);
    sscanf(message, "%[^:]: %[^\n]", left, right);
    return_strings[0] = left;
    return_strings[1] = right;
    return return_strings;
}


void sendAllGroupMessages(user *new_user) {
    int type_of_message = ROOM_MSG;
    char* directory_base = "logs/groups/";
    for(size_t i = 0; i < MAXGROUPS; i++) {
        char base_string[50];
        snprintf(base_string, sizeof(base_string), "%sGroup %zu", directory_base, i);
        FILE *fp = fopen(base_string, "r");

        if (fp == NULL) { continue; }

        char buf[username_length + message_length];

        char group_name[50];

        snprintf(group_name, sizeof(group_name), "Group %zu", i);

        while (fgets(buf, sizeof(buf), fp)) {
            char** string_split = parseGroupString(buf);
            send(new_user->sockid, &type_of_message, sizeof(type_of_message), 0);
            send(new_user->sockid, string_split[1], message_length, 0);
            send(new_user->sockid, string_split[0], username_length, 0);
            send(new_user->sockid, group_name, username_length, 0); // this is send full dir & not group name
            free(string_split[0]);
            free(string_split[1]);
            free(string_split);
        }
    }
}

size_t recv_exact_msg(void* buf, size_t len, int sock) {
	recieved_message* temp = (recieved_message*)buf;
	size_t total = 0;
	while (total < len) {
		size_t r = recv(sock, (temp) + total, len - total, 0);
		if (r == 0)  return 0;
		total += r;
	}
	return total;
}

//recieve all username packets
void recv_exact_username(char* temp, size_t len, int sock) {
    size_t total = 0;
    while(total < len) {
        size_t r = recv(sock, temp+total, len - total, 0);
        if (r == 0)  return; 
        total += r;
    }
}

void recv_exact_png(char* temp, uint32_t len, int sock) {
    ssize_t total = 0;
    while(total < len) {
        ssize_t r = recv(sock, temp+total, len - total, 0);
        if (r == 0)  return; 
        total += r;
    }
}

void sendSize(int size, thread_arg* threadArg, size_t index) {
    send(threadArg->user_Map->m_userArr[index]->sockid, &size, sizeof(int), 0);
}

void sendAll(char* msg, thread_arg* threadArg, int index, int size) {
    size_t total = 0;
    while(total < size) {
        size_t r = send(threadArg->user_Map->m_userArr[index]->sockid, msg+total, size - total, 0);
        total += r;
    }
}

void sendUsername(char username[50], int index, thread_arg* threadArg) {
    send(threadArg->user_Map->m_userArr[index]->sockid, username, 50, 0);
}

void sendPng(recieved_png* msg, thread_arg* threadArg) {
    pthread_mutex_lock(threadArg->mutex);
    size_t index = findUser(threadArg->user_Map, msg->user_to_send);
    pthread_mutex_unlock(threadArg->mutex);

    if(index > MAXUSERS) {
        printf("Index is wrong! \n");
        return;
    }

    int type_of_message = PNG_SEND;
    send(threadArg->user_Map->m_userArr[index]->sockid, &type_of_message, sizeof(type_of_message), 0);

    sendSize(msg->size_m,  threadArg, index);
    sendAll(msg->arr, threadArg, index, msg->size_m);
    sendUsername(threadArg->curr->username, index, threadArg);
    sendUsername(msg->filename_to_send, index, threadArg);
}

//wish we had templates in C
void write_to_file_group(message_s_group * message_to_send_group, char* username, pthread_mutex_t *group_fileMutex) {
    pthread_mutex_lock(group_fileMutex);

    char* filename = setupFileStringGroup(message_to_send_group->groupName); // one we are sending to

    FILE* fp = fopen(filename, "a");
    fseek(fp, 0, SEEK_END);
    fprintf(fp, "%s", username);
    fprintf(fp, " : ");
    fprintf(fp, "%s", message_to_send_group->arr);
    fprintf(fp, "\n");
    fclose(fp);
    free(filename);

    pthread_mutex_unlock(group_fileMutex);
}

void write_to_file_user(message_s * message_to_send_user, char* threadUsername, char* recvUsername, pthread_mutex_t *user_fileMutex) {
    pthread_mutex_lock(user_fileMutex);

    char* filename = setupFileStringUser(threadUsername, recvUsername); // one we are sending to
    FILE* fp = fopen(filename, "a");
    if(fp == NULL) {
        free(filename);
    }
    else {
        fseek(fp, 0, SEEK_END);
        fprintf(fp, "%s", threadUsername);
        fprintf(fp, " : ");
        fprintf(fp, "%s", message_to_send_user->arr);
        fprintf(fp, "\n");
        fclose(fp);
        free(filename);
    }

    if(strncmp(threadUsername, recvUsername, strlen(threadUsername)) == 0) {
        pthread_mutex_unlock(user_fileMutex);
        return;
    }

    char* filename2 = setupFileStringUser(recvUsername, threadUsername); // one we are sending to
    FILE* fp2 = fopen(filename2, "a");
    if(fp2 == NULL) {
        free(filename);
    }
    else {
        fseek(fp2, 0, SEEK_END);
        fprintf(fp2, "%s", threadUsername);
        fprintf(fp2, " : ");
        fprintf(fp2, "%s", message_to_send_user->arr);
        fprintf(fp2, "\n");
        fclose(fp2);
        free(filename2);
    }

    pthread_mutex_unlock(user_fileMutex);
}

void sendList(user_map* t_map) {
    client_list_s* client_list_send = malloc(sizeof(client_list_s));
    memset(client_list_send, 0, sizeof(client_list_s));
    int outer_index = 0;
    client_list_send->size = t_map->m_size;

    for(size_t j = 0; j < MAXUSERS; j++) {
        if(t_map->m_userArr[j] == NULL) {
            continue;
        }
        else {
            strcpy((client_list_send->arr[outer_index]), (t_map->m_userArr[j]->username));
            outer_index++;
        }
    }

    client_list_send->size = htonl(client_list_send->size);

    for(size_t i = 0; i < MAXUSERS; i++) {
        if(t_map->m_userArr[i] == NULL) {continue;}

        //make this into a function
        int type_of_message_list = MSG_LIST;
        send(t_map->m_userArr[i]->sockid, &type_of_message_list, sizeof(type_of_message_list), 0);

        send(t_map->m_userArr[i]->sockid, client_list_send, sizeof (client_list_s), 0);
    }

    free(client_list_send);
}

void send_chatroom_list(ChatRoomList* chatroom_list, int sockid) {
    ChatRoom* temp = chatroom_list->head;
    client_list_s *chatroom_list_send = malloc(sizeof(client_list_s));
    memset(chatroom_list_send, 0, sizeof(client_list_s));

    for(int i = 0; i < chatroom_list->size; i++) {
        strcpy((chatroom_list_send->arr[i]), (temp->ChatRoomName));
        temp = temp->next;
    }
    int type_of_message_list = ROOM_LIST;
    send(sockid, &type_of_message_list, sizeof(type_of_message_list), 0);

    chatroom_list_send->size = htonl(chatroom_list->size);
    send(sockid, chatroom_list_send, sizeof (client_list_s), 0);

    free(chatroom_list_send);
}

void room_method_message(recieved_message* a, user* temp, thread_arg* curr_user, int type_of_message, void* data, int size, thread_arg* threadArg) {

    message_s_group *message_to_send_group = (message_s_group*)(data); // copy recieved data into this struct
    user_map* t_map = curr_user->user_Map;

    a->size_m = ntohl(a->size_m);
    a->size_u = ntohl(a->size_u);

    strncpy(message_to_send_group->arr, a->arr, a->size_m);
    strncpy(message_to_send_group->groupName, a->user_to_send, a->size_u);
    strncpy(message_to_send_group->username, curr_user->curr->username, username_length);

    message_to_send_group->username[a->size_u] = '\0';
    message_to_send_group->arr[a->size_m] = '\0';

    for(size_t i = 0; i < MAXUSERS; i++) {
        if(t_map->m_userArr[i] == NULL) {continue;}

        if(t_map->m_userArr[i]->id == curr_user->curr->id) {continue;}

        send(t_map->m_userArr[i]->sockid, &type_of_message, sizeof(type_of_message), 0);

        send(t_map->m_userArr[i]->sockid, message_to_send_group, size, 0);
    }

    write_to_file_group(message_to_send_group, threadArg->curr->username, threadArg->group_fileMutex);

}

void room_method_creation(user* temp, thread_arg* curr_user, int type_of_message, void* data, int size) {
    data = (char*)data;
    user_map* t_map = curr_user->user_Map;

    for(size_t i = 0; i < MAXUSERS; i++) {
        if(t_map->m_userArr[i] == NULL) {continue;}

        if(t_map->m_userArr[i]->id == curr_user->curr->id) {continue;}
        
        send(t_map->m_userArr[i]->sockid, &type_of_message, sizeof(type_of_message), 0);

        send(t_map->m_userArr[i]->sockid, data, size, 0);
    }
}

//username is the user who is sending message
void send_message_user(message_s *message_to_send, int current_user_socket, thread_arg* threadArg) { // username
    recieved_message recievedMessage;
    recv_exact_msg(&recievedMessage, sizeof(recieved_message), current_user_socket);
    
    recievedMessage.size_m = ntohl(recievedMessage.size_m);
    recievedMessage.size_u = ntohl(recievedMessage.size_u);

    strncpy(message_to_send->arr, recievedMessage.arr, message_length);
    strncpy(message_to_send->username, threadArg->curr->username, username_length); // this is our username

    recievedMessage.user_to_send[recievedMessage.size_u] = '\0';
    recievedMessage.arr[recievedMessage.size_m] = '\0';

    write_to_file_user(message_to_send, threadArg->curr->username, recievedMessage.user_to_send, threadArg->user_fileMutex);

    size_t index = findUser(threadArg->user_Map, recievedMessage.user_to_send);

    //this is the type, letting the client know we are sending a message
    int type_of_message = MSG_SEND;
    send(threadArg->user_Map->m_userArr[index]->sockid, &type_of_message, sizeof(type_of_message), 0);

    int sent = send(threadArg->user_Map->m_userArr[index]->sockid, message_to_send, sizeof(message_s), 0);

    printf("Sent to the new client: %d\n", sent);

    memset(message_to_send, 0, sizeof(message_s));
}

void setupDir(char* username) {
    size_t len = strlen("logs/users/") + strlen(username) + 1;
    char* dir_location = malloc(len);
    snprintf(dir_location, len, "logs/users/%s", username);
    mkdir(dir_location, 0776);
    free(dir_location);
}

char* setupFileStringUser(char *username, char* username_to_send) {
    size_t len = strlen("logs/users/") + strlen(username) + 1 + strlen(username_to_send) + 4 + 1;
    char* file_location = malloc(len);
    snprintf(file_location, len, "logs/users/%s/%s.txt", username, username_to_send);
    return file_location;
}

char* setupFileStringGroup(char* group) {
    size_t len = strlen("logs/groups/") + strlen(group) + 1;
    char* file_location = malloc(len);
    snprintf(file_location, len, "logs/groups/%s.txt", group);
    return file_location;
}

void sendUserRemoval(thread_arg* threadArg) {
    user_map* t_map = threadArg->user_Map;
    int type_of_message_list = USER_EXIT;
    for(int i = 0; i < MAXUSERS; i++) {
        if(t_map->m_userArr[i] == NULL || t_map->m_userArr[i]->id == threadArg->curr->id) {continue;}
        send(t_map->m_userArr[i]->sockid, &type_of_message_list, sizeof(type_of_message_list), 0);

        send(t_map->m_userArr[i]->sockid, threadArg->curr->username, sizeof(threadArg->curr->username), 0);
    }
}

void init_file_data_structure(recieved_png* png, uint32_t png_size) {
    png->arr = malloc(png_size);
    memset(png->arr, 0, png_size);
}

uint32_t recv_file_size(thread_arg* arg) {
    uint32_t png_size = 0;
    recv(arg->curr->sockid, &png_size, sizeof(uint32_t), 0);
    return png_size;
}

void process_file(recieved_png* png, uint32_t png_size) {
    png->user_to_send[49] = '\0';
    png->size_m = png_size;
    png->size_u = strlen(png->user_to_send);
}

void send_file(thread_arg* arg) {
    uint32_t png_size = recv_file_size(arg);
    recieved_png png;
    init_file_data_structure(&png,png_size);
    recv_exact_png(png.arr, png_size, arg->curr->sockid);
    recv_exact_username(png.user_to_send, 50, arg->curr->sockid); // user to send to
    recv_exact_username(png.filename_to_send, 50, arg->curr->sockid); // filename
    process_file(&png, png_size);
    sendPng(&png, arg);
    free(png.arr);
}

void sendPngGroup(recieved_png* msg, thread_arg* threadArg) {
    int type_of_message = FILE_GROUP;
    for(int i = 0; i < MAXGROUPS; i++) {
        if(threadArg->user_Map->m_userArr[i] == NULL) {continue;}
        if(threadArg->user_Map->m_userArr[i]->sockid == threadArg->curr->sockid) {continue;}
        send(threadArg->user_Map->m_userArr[i]->sockid, &type_of_message, sizeof(type_of_message), 0);
        sendSize(msg->size_m,  threadArg, i);
        sendAll(msg->arr, threadArg, i, msg->size_m);
        sendUsername(threadArg->curr->username, i, threadArg);
        sendUsername(msg->filename_to_send, i, threadArg);
        sendUsername(msg->user_to_send, i, threadArg);
    }
}

void send_file_group(thread_arg* arg) {
    uint32_t png_size = recv_file_size(arg);
    recieved_png png;
    init_file_data_structure(&png,png_size);
    recv_exact_png(png.arr, png_size, arg->curr->sockid);
    recv_exact_username(png.user_to_send, 50, arg->curr->sockid); // user to send to
    recv_exact_username(png.filename_to_send, 50, arg->curr->sockid); // filename
    process_file(&png, png_size);
    sendPngGroup(&png, arg);
    free(png.arr);
}

void *create_connection(void *arg) {
        int n;
        MsgHeader hdr;

        message_s *message_to_send = (message_s*) malloc(sizeof(message_s));
        message_s_group *message_to_send_group = (message_s_group*) malloc(sizeof(message_s_group));

        thread_arg* curr_user = (thread_arg*)arg;

        setupDir(curr_user->curr->username);

        int current_user_socket = curr_user->curr->sockid;

        pthread_mutex_lock(curr_user->mutex);
        print_client_list(curr_user->list_of_users);
        pthread_mutex_unlock(curr_user->mutex);

        printf("Connection Established!\n");
        printf("IP: %d \n", curr_user->curr->client.sin_addr.s_addr);

        while((n = recv(current_user_socket, &hdr, sizeof(hdr), 0)) > 0) {

            uint32_t type   = ntohl(hdr.type);
            uint32_t length = ntohl(hdr.length);

            //the client is sending us information
            if(type == MSG_SEND) {
                send_message_user(message_to_send, current_user_socket, curr_user);
            }

            else if(type == MSG_EXIT) {
                printf("Closing Connection. \n");
                break;
            }

            else if(type == ROOM_CREATE) {
                ChatRoom* newRoom = malloc(sizeof(ChatRoom));

                //since username is same size as chatroom name, we can simply use this function again
                recv_exact_username(newRoom->ChatRoomName, 50, current_user_socket);

                pthread_mutex_lock(curr_user->mutex);
                insert_ChatRoom(curr_user->ChatRoom_list, newRoom);
                pthread_mutex_unlock(curr_user->mutex);
                
                room_method_creation(curr_user->list_of_users->head, curr_user, ROOM_CREATE, newRoom->ChatRoomName, 50);
            }

            else if(type == ROOM_MSG) {
                recieved_message a = {0};

                recv_exact_msg(&a, sizeof(recieved_message), current_user_socket);
                
                room_method_message(&a, curr_user->list_of_users->head, curr_user, ROOM_MSG, message_to_send_group, 228, curr_user);
            }
            else if(type == PNG_SEND) {
                send_file(curr_user);
            }
            else if(type == FILE_GROUP) {
                send_file_group(curr_user);
            }
        }

        close(curr_user->curr->sockid);

        pthread_mutex_lock(curr_user->mutex);

        removeUser(curr_user->user_Map, curr_user->curr);

        sendUserRemoval(curr_user);

        pthread_mutex_unlock(curr_user->mutex);

        free(arg);
        free(message_to_send);
        free(message_to_send_group);
        pthread_exit(NULL);
}