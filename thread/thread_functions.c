#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "thread_functions.h"
#include "../user/user_list.h"
#include "../messages/messages.h"
#define MSG_SEND 1
#define MSG_LIST 2
#define MSG_EXIT 3
#define USER_EXIT 4
#define ROOM_CREATE 5
#define ROOM_MSG 6
#define ROOM_LIST 7
#define FILE_SEND 8
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

size_t recvExactMsg(void* buf, size_t len, int sock) {
	recieved_message* temp = (recieved_message*)buf;
	size_t total = 0;
	while (total < len) {
		size_t r = recv(sock, (temp) + total, len - total, 0);
		if (r == 0)  return 0;
		total += r;
	}
	return total;
}

void recvExactUsername(char* temp, size_t len, int sock) {
    size_t total = 0;
    while(total < len) {
        size_t r = recv(sock, temp+total, len - total, 0);
        if (r == 0)  return; 
        total += r;
    }
}

void recvExactPng(char* temp, uint32_t len, int sock) {
    ssize_t total = 0;
    while(total < len) {
        ssize_t r = recv(sock, temp+total, len - total, 0);
        if (r == 0)  return; 
        total += r;
    }
}

void sendSize(int size, int sockid) {
    send(sockid, &size, sizeof(int), 0);
}

void sendAll(char* msg, int sockid, int size) {
    int total = 0;
    while(total < size) {
        int r = send(sockid, msg+total, size - total, 0);
        total += r;
    }
}

void sendUsername(char username[50], int sockid) {
    send(sockid, username, 50, 0);
}

void sendPng(recieved_png* msg, thread_arg* threadArg) {
    pthread_mutex_lock(threadArg->mutex);
    user_info info = findUser(threadArg->user_Map, msg->user_to_send);
    pthread_mutex_unlock(threadArg->mutex);

    if(info.mutex == NULL) {
        printf("Index is wrong! \n");
        return;
    }

    int type_of_message = FILE_SEND;

    pthread_mutex_lock(info.mutex);
    send(info.sockid, &type_of_message, sizeof(type_of_message), 0);
    sendSize(msg->size_m, info.sockid);
    sendAll(msg->arr, info.sockid, msg->size_m);
    sendUsername(threadArg->curr->username, info.sockid);
    sendUsername(msg->filename_to_send, info.sockid);
    pthread_mutex_unlock(info.mutex);
}

//wish we had templates in C
void writeToFileGroup(message_s_group * message_to_send_group, char* username, pthread_mutex_t *group_fileMutex) {
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

void writeToFileUser(recieved_message* message_to_send_user, char* threadUsername, char* recvUsername, pthread_mutex_t *user_fileMutex) {
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

void sendChatroomList(ChatRoomList* chatroom_list, int sockid) {
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

void roomMethodMessage(thread_arg* curr_user, int type_of_message, void* data, int size, thread_arg* threadArg) {
    recieved_message a = {0};

    recvExactMsg(&a, sizeof(recieved_message), curr_user->curr->sockid);

    message_s_group *message_to_send_group = (message_s_group*)(data); // copy recieved data into this struct
    user_map* t_map = curr_user->user_Map;

    a.size_m = ntohl(a.size_m);
    a.size_u = ntohl(a.size_u);

    strncpy(message_to_send_group->arr, a.arr, a.size_m);
    strncpy(message_to_send_group->groupName, a.user_to_send, a.size_u);
    strncpy(message_to_send_group->username, curr_user->curr->username, username_length);

    message_to_send_group->username[a.size_u] = '\0';
    message_to_send_group->arr[a.size_m] = '\0';

    pthread_mutex_lock(curr_user->mutex);
    for(size_t i = 0; i < MAXUSERS; i++) {
        if(t_map->m_userArr[i] == NULL) {continue;}

        if(t_map->m_userArr[i]->id == curr_user->curr->id) {continue;}

        pthread_mutex_lock(t_map->m_userArr[i]->user_mutex);
        send(t_map->m_userArr[i]->sockid, &type_of_message, sizeof(type_of_message), 0);
        send(t_map->m_userArr[i]->sockid, message_to_send_group, size, 0);
        pthread_mutex_unlock(t_map->m_userArr[i]->user_mutex);
    }
    pthread_mutex_unlock(curr_user->mutex);

    writeToFileGroup(message_to_send_group, threadArg->curr->username, threadArg->group_fileMutex);
}

void roomMethodCreation(thread_arg* curr_user, int type_of_message, void* data, int size) {
    data = (char*)data;
    user_map* t_map = curr_user->user_Map;

    pthread_mutex_lock(curr_user->mutex);
    for(size_t i = 0; i < MAXUSERS; i++) {
        if(t_map->m_userArr[i] == NULL) {continue;}

        if(t_map->m_userArr[i]->id == curr_user->curr->id) {continue;}
        
        pthread_mutex_lock(t_map->m_userArr[i]->user_mutex);
        send(t_map->m_userArr[i]->sockid, &type_of_message, sizeof(type_of_message), 0);
        send(t_map->m_userArr[i]->sockid, data, size, 0);
        pthread_mutex_unlock(t_map->m_userArr[i]->user_mutex);
    }
    pthread_mutex_unlock(curr_user->mutex);
    free(data);
}

//username is the user who is sending message
void sendMessageUser(int current_user_socket, thread_arg* threadArg) { // username
    recieved_message recievedMessage;

    recievedMessage.size_m = recvSize(threadArg);
    recvExactMsg(recievedMessage.arr, recievedMessage.size_m , current_user_socket);
    
    recievedMessage.size_u = recvSize(threadArg);
    recvExactMsg(recievedMessage.user_to_send, recievedMessage.size_u , current_user_socket);

    writeToFileUser(&recievedMessage, threadArg->curr->username, recievedMessage.user_to_send, threadArg->user_fileMutex);

    pthread_mutex_lock(threadArg->mutex);
    user_info info = findUser(threadArg->user_Map, recievedMessage.user_to_send);
    pthread_mutex_unlock(threadArg->mutex);

    if(info.sockid == 0 || info.mutex == NULL) {
        printf("CANNOT FIND USER! \n");
        return;
    }

    strncpy(recievedMessage.user_to_send, threadArg->curr->username, strlen(threadArg->curr->username) + 1);

    pthread_mutex_lock(info.mutex);
    sendMessage(&recievedMessage, info.sockid);
    pthread_mutex_unlock(info.mutex);

    printf("Sent to the new client\n");
}

void sendMessage(recieved_message* message_struct, int socket_id) {
    int type_of_message = MSG_SEND;
    send(socket_id, &type_of_message, sizeof(type_of_message), 0);
    send(socket_id, &(message_struct->size_m), sizeof(uint32_t), 0);
    send(socket_id, (message_struct->arr), message_struct->size_m, 0);
    send(socket_id, &(message_struct->size_u), sizeof(uint32_t), 0);
    send(socket_id, (message_struct->user_to_send), message_struct->size_u, 0);
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

void initFileDataStructure(recieved_png* png, uint32_t png_size) {
    png->arr = malloc(png_size);
    memset(png->arr, 0, png_size);
}

uint32_t recvSize(thread_arg* arg) {
    uint32_t png_size = 0;
    recv(arg->curr->sockid, &png_size, sizeof(uint32_t), 0);
    return ntohl(png_size);
}

void processFile(recieved_png* png, uint32_t png_size) {
    png->user_to_send[49] = '\0';
    png->size_m = png_size;
    png->size_u = strlen(png->user_to_send);
}

void sendFileUser(thread_arg* arg) {
    uint32_t png_size = recvSize(arg);
    recieved_png png;
    initFileDataStructure(&png, png_size);
    recvExactMsg(png.arr, png_size, arg->curr->sockid);
    recvExactUsername(png.user_to_send, 50, arg->curr->sockid); // user to send to
    recvExactUsername(png.filename_to_send, 50, arg->curr->sockid); // filename
    processFile(&png, png_size);
    sendPng(&png, arg);
    free(png.arr);
}

void sendPngGroup(recieved_png* msg, thread_arg* threadArg) {
    int type_of_message = FILE_GROUP;

    pthread_mutex_lock(threadArg->mutex);

    for(int i = 0; i < MAXGROUPS; i++) {
        if(threadArg->user_Map->m_userArr[i] == NULL) {continue;}
        if(threadArg->user_Map->m_userArr[i]->sockid == threadArg->curr->sockid) {continue;}

        pthread_mutex_lock(threadArg->user_Map->m_userArr[i]->user_mutex);
        int sockid = threadArg->user_Map->m_userArr[i]->sockid;
        send(sockid, &type_of_message, sizeof(type_of_message), 0);
        sendSize(msg->size_m,  sockid);
        sendAll(msg->arr, sockid, msg->size_m);
        sendUsername(threadArg->curr->username, sockid);
        sendUsername(msg->filename_to_send, sockid);
        sendUsername(msg->user_to_send, sockid);
        pthread_mutex_unlock(threadArg->user_Map->m_userArr[i]->user_mutex);
    }
    pthread_mutex_unlock(threadArg->mutex);
}

void sendFileGroup(thread_arg* arg) {
    uint32_t png_size = recvSize(arg);
    recieved_png png;
    initFileDataStructure(&png,png_size);

    recvExactMsg(png.arr, png_size, arg->curr->sockid);
    recvExactUsername(png.user_to_send, 50, arg->curr->sockid); // user to send to
    recvExactUsername(png.filename_to_send, 50, arg->curr->sockid); // filename
    processFile(&png, png_size);
    sendPngGroup(&png, arg);
    free(png.arr);
}

void handleRoomCreation(thread_arg* curr_user, int current_user_socket) {
    ChatRoom* newRoom = malloc(sizeof(ChatRoom));
    recvExactUsername(newRoom->ChatRoomName, 50, current_user_socket);

    pthread_mutex_lock(curr_user->mutex);
    insert_ChatRoom(curr_user->ChatRoom_list, newRoom);
    pthread_mutex_unlock(curr_user->mutex);
                
    roomMethodCreation(curr_user, ROOM_CREATE, newRoom->ChatRoomName, 50);
}

void *createConnection(void *arg) {
        int n;
        MsgHeader hdr;

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
                sendMessageUser(current_user_socket, curr_user);
            }

            else if(type == MSG_EXIT) {
                printf("Closing Connection. \n");
                break;
            }

            else if(type == ROOM_CREATE) {
                handleRoomCreation(curr_user, current_user_socket);
            }
            else if(type == ROOM_MSG) {
                roomMethodMessage(curr_user, ROOM_MSG, message_to_send_group, 228, curr_user);
            }
            else if(type == FILE_SEND) {
                sendFileUser(curr_user);
            }
            else if(type == FILE_GROUP) {
                sendFileGroup(curr_user);
            }
        }

        close(curr_user->curr->sockid);

        pthread_mutex_lock(curr_user->mutex);

        removeUser(curr_user->user_Map, curr_user->curr);

        sendUserRemoval(curr_user);

        pthread_mutex_unlock(curr_user->mutex);

        free(arg);
        free(message_to_send_group);
        pthread_exit(NULL);
}