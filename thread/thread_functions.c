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
#define USERNAME_LENGTH 50
#define message_length 128

char** parseGroupString(char message[USERNAME_LENGTH + message_length]) {
    char** return_strings = malloc(2);
    char* left = malloc(USERNAME_LENGTH);
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

        char buf[USERNAME_LENGTH + message_length];

        char group_name[50];

        snprintf(group_name, sizeof(group_name), "Group %zu", i);

        while (fgets(buf, sizeof(buf), fp)) {
            char** string_split = parseGroupString(buf);
            send(new_user->sockid, &type_of_message, sizeof(type_of_message), 0);
            send(new_user->sockid, string_split[1], message_length, 0);
            send(new_user->sockid, string_split[0], USERNAME_LENGTH, 0);
            send(new_user->sockid, group_name, USERNAME_LENGTH, 0); // this is send full dir & not group name
            free(string_split[0]);
            free(string_split[1]);
            free(string_split);
        }
    }
}

size_t recvExactMsg(void* buf, size_t len, int sock) {
	char* temp = (char*)buf;
	size_t total = 0;
	while (total < len) {
		size_t r = recv(sock, (temp) + total, len - total, 0);
		if (r == 0)  return 0;
		total += r;
	}
	return total;
}

size_t recvExactUsername(char* temp, int sock) {
    size_t len = recvSize(sock);
    size_t total = 0;
    while(total < len) {
        size_t r = recv(sock, temp+total, len - total, 0);
        if (r == 0) {
            return 0;
        }
        total += r;
    }
    return len;
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
    size = htonl(size);
    send(sockid, &size, sizeof(int), 0);
}

void sendAll(char* msg, int sockid, int size) {
    int total = 0;
    while(total < size) {
        int r = send(sockid, msg+total, size - total, 0);
        total += r;
    }
}

void sendUsername(char* username, int size, int sockid) {
    sendSize(size, sockid);
    send(sockid, username, size, 0);
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
    sendUsername(threadArg->curr->username, threadArg->curr->username_length, info.sockid);
    sendUsername(msg->filename_to_send, msg->size_f_name, info.sockid);
    pthread_mutex_unlock(info.mutex);
}

//wish we had templates in C
void writeToFileGroup(recieved_message* message_to_send_group, char* username, pthread_mutex_t *group_fileMutex) {
    pthread_mutex_lock(group_fileMutex);

    char* filename = setupFileStringGroup(message_to_send_group->user_to_send); // one we are sending to

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

    for(size_t i = 0; i < MAXUSERS; i++) {
        if(t_map->m_userArr[i] == NULL) {continue;}

        //make this into a function
        int type_of_message_list = MSG_LIST;
        send(t_map->m_userArr[i]->sockid, &type_of_message_list, sizeof(type_of_message_list), 0);

        sendSize(client_list_send->size, t_map->m_userArr[i]->sockid);

        for(int j = 0; j < client_list_send->size; j++) {
            sendUsername(client_list_send->arr[j], strlen(client_list_send->arr[j]) + 1, t_map->m_userArr[i]->sockid);
        }
    }
    free(client_list_send);
}

void sendChatroomList(ChatRoomList* chatroom_list, int sockid) {
    ChatRoom* temp = chatroom_list->head;

    int type_of_message_list = ROOM_LIST;
    send(sockid, &type_of_message_list, sizeof(type_of_message_list), 0);

    sendSize(chatroom_list->size, sockid);

    for(int i = 0; i < chatroom_list->size; i++) {
        sendUsername(temp->ChatRoomName, strlen(temp->ChatRoomName) + 1, sockid);
        temp = temp -> next;
    }
}

void roomMethodMessage(thread_arg* curr_user) {
    recieved_message recievedMessage = {0};

    recievedMessage.size_m = recvSize(curr_user->curr->sockid);
    recvExactMsg(recievedMessage.arr, recievedMessage.size_m , curr_user->curr->sockid);
    
    recievedMessage.size_u = recvSize(curr_user->curr->sockid);
    recvExactMsg(recievedMessage.user_to_send, recievedMessage.size_u , curr_user->curr->sockid);

    char* group = malloc(recievedMessage.size_u);
    memcpy(group, recievedMessage.user_to_send, recievedMessage.size_u);
    group[recievedMessage.size_u - 1] = '\0';

    memcpy(recievedMessage.user_to_send, curr_user->curr->username, strlen(curr_user->curr->username));
    recievedMessage.user_to_send[strlen(curr_user->curr->username) + 1] = '\0';
    recievedMessage.size_u = strlen(curr_user->curr->username) + 1;

    user_map* t_map = curr_user->user_Map;

    int group_length = htonl(strlen(group) + 1);

    recievedMessage.size_m = htonl(recievedMessage.size_m);
    recievedMessage.size_u = htonl(recievedMessage.size_u);

    pthread_mutex_lock(curr_user->mutex);
    for(size_t i = 0; i < MAXUSERS; i++) {
        if(t_map->m_userArr[i] == NULL) {continue;}

        if(t_map->m_userArr[i]->id == curr_user->curr->id) {continue;}

        pthread_mutex_lock(t_map->m_userArr[i]->user_mutex);
        sendMessage(&recievedMessage, t_map->m_userArr[i]->sockid, ROOM_MSG);
        send(t_map->m_userArr[i]->sockid, &group_length, sizeof(int), 0);
        send(t_map->m_userArr[i]->sockid, group, ntohl(group_length), 0);
        pthread_mutex_unlock(t_map->m_userArr[i]->user_mutex);
    }
    pthread_mutex_unlock(curr_user->mutex);

    free(group);
    writeToFileGroup(&recievedMessage, curr_user->curr->username, curr_user->group_fileMutex);
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
        sendUsername(data, size, t_map->m_userArr[i]->sockid);
        pthread_mutex_unlock(t_map->m_userArr[i]->user_mutex);
    }
    pthread_mutex_unlock(curr_user->mutex);
    //free(data);
}

//username is the user who is sending message
void sendMessageUser(int current_user_socket, thread_arg* threadArg) { // username
    recieved_message recievedMessage;

    recievedMessage.size_m = recvSize(threadArg->curr->sockid);
    recvExactMsg(recievedMessage.arr, recievedMessage.size_m , current_user_socket);
    
    recievedMessage.size_u = recvSize(threadArg->curr->sockid);
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
    recievedMessage.size_m = htonl(recievedMessage.size_m);
    recievedMessage.size_u = htonl(recievedMessage.size_u);

    pthread_mutex_lock(info.mutex);
    sendMessage(&recievedMessage, info.sockid, MSG_SEND);
    pthread_mutex_unlock(info.mutex);

    printf("Sent to the new client\n");
}

void sendMessage(recieved_message* message_struct, int socket_id, int type_of_message) {
    send(socket_id, &type_of_message, sizeof(type_of_message), 0);
    send(socket_id, &(message_struct->size_m), sizeof(uint32_t), 0);
    send(socket_id, (message_struct->arr), ntohl(message_struct->size_m), 0);
    send(socket_id, &(message_struct->size_u), sizeof(uint32_t), 0);
    send(socket_id, (message_struct->user_to_send), ntohl(message_struct->size_u), 0);
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
        sendUsername(threadArg->curr->username, strlen(threadArg->curr->username) + 1, t_map->m_userArr[i]->sockid);
    }
}

void initFileDataStructure(recieved_png* png, uint32_t png_size) {
    png->arr = malloc(png_size);
    memset(png->arr, 0, png_size);
}

uint32_t recvSize(int sockid) {
    uint32_t png_size = 0;
    recv(sockid, &png_size, sizeof(uint32_t), 0);
    return ntohl(png_size);
}

void processFile(recieved_png* png, uint32_t png_size) {
    png->user_to_send[49] = '\0';
    png->size_m = png_size;
    png->size_u = strlen(png->user_to_send) + 1;
    png->size_f_name = strlen(png->filename_to_send) + 1;
}



void sendFileUser(thread_arg* arg) {
    uint32_t png_size = recvSize(arg->curr->sockid);
    recieved_png png;
    initFileDataStructure(&png, png_size);
    recvExactMsg(png.arr, png_size, arg->curr->sockid);
    recvExactUsername(png.user_to_send, arg->curr->sockid); // user to send to
    recvExactUsername(png.filename_to_send, arg->curr->sockid); // filename
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
        sendUsername(threadArg->curr->username, strlen(threadArg->curr->username) + 1, sockid);
        sendUsername(msg->filename_to_send, msg->size_f_name, sockid);
        sendUsername(msg->user_to_send, msg->size_u + 1, sockid);
        pthread_mutex_unlock(threadArg->user_Map->m_userArr[i]->user_mutex);
    }
    pthread_mutex_unlock(threadArg->mutex);
}

void sendFileGroup(thread_arg* arg) {
    uint32_t png_size = recvSize(arg->curr->sockid);
    recieved_png png;
    initFileDataStructure(&png,png_size);

    recvExactMsg(png.arr, png_size, arg->curr->sockid);
    recvExactUsername(png.user_to_send, arg->curr->sockid); // user to send to
    recvExactUsername(png.filename_to_send, arg->curr->sockid); // filename
    png.size_f_name = strlen(png.filename_to_send) + 1;
    processFile(&png, png_size);
    sendPngGroup(&png, arg);
    free(png.arr);
}

void handleRoomCreation(thread_arg* curr_user, int current_user_socket) {
    ChatRoom* newRoom = malloc(sizeof(ChatRoom));
    uint32_t length = recvExactUsername(newRoom->ChatRoomName, current_user_socket);
    newRoom->name_length = length;

    pthread_mutex_lock(curr_user->mutex);
    insert_ChatRoom(curr_user->ChatRoom_list, newRoom);
    pthread_mutex_unlock(curr_user->mutex);
                
    roomMethodCreation(curr_user, ROOM_CREATE, newRoom->ChatRoomName, strlen(newRoom->ChatRoomName) + 1);
}

void *createConnection(void *arg) {
        int n;
        uint32_t hdr;

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

            uint32_t type   = ntohl(hdr);

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
                roomMethodMessage(curr_user);
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