#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
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
#define USER_JOIN 10
#define FILE_DOWNLOAD 11
#define USER_CHATS 12
#define USERNAME_LENGTH 50
#define message_length 128

void sendAllUserFiles(user* user) {
    int type_of_message = FILE_SEND;
    char base_string[128] = "logs/files/";
    struct dirent *entry;
    
    DIR *dp = opendir(base_string);

    if(dp == NULL) {return;}

    while ((entry = readdir(dp)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        char inner_path[256];
        sprintf(inner_path, "%s/%s", base_string, entry->d_name);

        DIR *dp_inner = opendir(inner_path);
        if(dp_inner == NULL) {return;}

        struct dirent* inner_entry;
        while ((inner_entry = readdir(dp_inner)) != NULL) {
            if (strcmp(inner_entry->d_name, ".") == 0 || strcmp(inner_entry->d_name, "..") == 0)
                continue;

            if(strncmp(inner_entry->d_name, user->username, user->username_length) == 0) {
                char inner_inner_path[256];
                sprintf(inner_inner_path, "%s/%s", inner_path, inner_entry->d_name);

                DIR *dp_inner_inner = opendir(inner_inner_path);
                if(dp_inner_inner == NULL) {return;}

                struct dirent* inner_inner_entry;
                while((inner_inner_entry = readdir(dp_inner_inner)) != NULL) {
                    if (strcmp(inner_inner_entry->d_name, ".") == 0 || strcmp(inner_inner_entry->d_name, "..") == 0)
                        continue;
                    char filename[50];
                    strncpy(filename, inner_inner_entry->d_name, strlen(inner_inner_entry->d_name));
                    filename[strlen(inner_inner_entry->d_name)] = '\0';
                    send(user->sockid, &type_of_message, sizeof(type_of_message), 0);
                    sendUsername(inner_inner_entry->d_name, strlen(inner_inner_entry->d_name) + 1, user->sockid);
                    sendUsername(inner_inner_entry->d_name, strlen(inner_inner_entry->d_name) + 1, user->sockid);
                }
                closedir(dp_inner_inner);
            }
        }
        closedir(dp_inner);
    }
    closedir(dp);
}

void sendPrevConnectedUserMessages(user* user) {
    // 1. Send file name (its the users name)
    // 2. Send all file content
    // 3. Send "END" string to signify end
    int type_of_message = USER_CHATS;
    char base_string[50];
    char buf[USERNAME_LENGTH + message_length];
    snprintf(base_string, sizeof(base_string), "logs/users/%s", user->username);

    struct dirent *entry;
    
    DIR *dp = opendir(base_string);

    if(dp == NULL) {return;}

    send(user->sockid, &(type_of_message), sizeof(int), 0);

    while ((entry = readdir(dp)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char path[256];
        char filename[50];
        strncpy(filename, entry->d_name, strlen(entry->d_name) - 4);
        filename[strlen(entry->d_name) - 3] = '\0';
        filename[strlen(entry->d_name) - 2] = '\0';
        snprintf(path, sizeof(path), "%s/%s", base_string, entry->d_name);
        FILE *fp = fopen(path, "r");

        if (fp == NULL) { continue; }
        sendUsername("FILE", 5, user->sockid); // indicate the start of a file
        sendUsername(filename, strlen(filename) + 1, user->sockid); // filename (username)

        while (fgets(buf, sizeof(buf), fp)) {
            char** string_split = parseFileString(buf);

            recieved_message message_struct = {0};
            message_struct.arr = string_split[1];
            message_struct.size_m = htonl(strlen(string_split[1]) + 1);
            message_struct.user_to_send = string_split[0];
            message_struct.size_u = htonl(strlen(string_split[0]) + 1);

            send(user->sockid, &(message_struct.size_m), sizeof(uint32_t), 0);
            send(user->sockid, (message_struct.arr), ntohl(message_struct.size_m), 0);
            send(user->sockid, &(message_struct.size_u), sizeof(uint32_t), 0);
            send(user->sockid, (message_struct.user_to_send), ntohl(message_struct.size_u), 0);

            free(string_split[0]);
            free(string_split[1]);
            free(string_split);
        }
        fclose(fp);
    }

    sendUsername("END", 4, user->sockid); // END string
    closedir(dp);
}

char** parseFileString(char message[USERNAME_LENGTH + message_length]) {
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

        int group_name_length = htonl(strlen(group_name) + 1);

        while (fgets(buf, sizeof(buf), fp)) { // each line
            char** string_split = parseFileString(buf);

            recieved_message recvMsg = {0};
            recvMsg.arr = string_split[1];
            recvMsg.size_m = htonl(strlen(string_split[1]) + 1);
            recvMsg.user_to_send = string_split[0];
            recvMsg.size_u = htonl(strlen(string_split[0]) + 1);

            sendMessage(&recvMsg, new_user->sockid, type_of_message);
            send(new_user->sockid, &group_name_length, sizeof(int), 0);
            send(new_user->sockid, group_name, ntohl(group_name_length), 0); // this is send full dir & not group name

            free(string_split[0]);
            free(string_split[1]);
            free(string_split);
        }
        fclose(fp);
    }
}

char* recvExactMsg(uint32_t* len, int sock) {
    size_t length = recvSize(sock);
    *len = length;
    char* buf = malloc(length + 1);
	size_t total = 0;
	while (total < length) {
		size_t r = recv(sock, buf + total, length - total, 0);
		if (r == 0)  return 0;
		total += r;
	}
	return buf;
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

void sendFile(recievedFile* file, thread_arg* threadArg) {
    pthread_mutex_lock(threadArg->mutex);
    user_info info = findUser(threadArg->user_Map, file->user_to_send);
    pthread_mutex_unlock(threadArg->mutex);

    if(info.mutex == NULL) {
        printf("Index is wrong! \n");
        return;
    }

    int type_of_message = FILE_SEND;
    pthread_mutex_lock(info.mutex);

    send(info.sockid, &type_of_message, sizeof(type_of_message), 0);
    sendUsername(threadArg->curr->username, threadArg->curr->username_length, info.sockid);
    sendUsername(file->filename_to_send, file->size_f_name, info.sockid);

    pthread_mutex_unlock(info.mutex);
}

//wish we had templates in C
void writeToFileGroup(recieved_message* message_to_send_group, char* group, char* username, pthread_mutex_t *group_fileMutex) {
    pthread_mutex_lock(group_fileMutex);

    char* filename = setupFileStringGroup(group); // one we are sending to

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
        fprintf(fp, ": ");
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

void sendList(user_map* t_map, int sockid, pthread_mutex_t* socket_mutex) {
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

    pthread_mutex_lock(socket_mutex);

    int type_of_message_list = MSG_LIST;
    send(sockid, &type_of_message_list, sizeof(type_of_message_list), 0);

    sendSize(client_list_send->size, sockid);

    for(int j = 0; j < client_list_send->size; j++) {
        sendUsername(client_list_send->arr[j], strlen(client_list_send->arr[j]) + 1, sockid);
    }
    
    pthread_mutex_unlock(socket_mutex);
    free(client_list_send);
}

void sendUserJoin(user_map* t_map, user* new_user) {
    int type_of_message_list = USER_JOIN;
    for(size_t j = 0; j < MAXUSERS; j++) {
        if(t_map->m_userArr[j] == NULL || t_map->m_userArr[j] == new_user) {
            continue;
        }
        else {
            pthread_mutex_lock(t_map->m_userArr[j]->user_mutex);

            send(t_map->m_userArr[j]->sockid, &type_of_message_list, sizeof(type_of_message_list), 0);
            sendUsername(new_user->username, strlen(new_user->username) + 1, t_map->m_userArr[j]->sockid);

            pthread_mutex_unlock(t_map->m_userArr[j]->user_mutex);
        }
    }
}

void sendChatroomList(ChatRoomList* chatroom_list, int sockid, pthread_mutex_t* socket_mutex) {
    ChatRoom* temp = chatroom_list->head;

    pthread_mutex_lock(socket_mutex);
    int type_of_message_list = ROOM_LIST;
    send(sockid, &type_of_message_list, sizeof(type_of_message_list), 0);

    sendSize(chatroom_list->size, sockid);

    for(int i = 0; i < chatroom_list->size; i++) {
        sendUsername(temp->ChatRoomName, strlen(temp->ChatRoomName) + 1, sockid);
        temp = temp -> next;
    }
    pthread_mutex_unlock(socket_mutex);
}

void roomMethodMessage(thread_arg* curr_user) {
    recieved_message recievedMessage = {0};

    recievedMessage.arr = recvExactMsg(&recievedMessage.size_m, curr_user->curr->sockid);

    uint32_t group_size = 0;
    char* group = recvExactMsg(&group_size , curr_user->curr->sockid);
    group[group_size] = '\0';

    recievedMessage.user_to_send = malloc(strlen(curr_user->curr->username) + 2);
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

    writeToFileGroup(&recievedMessage, group, curr_user->curr->username, curr_user->group_fileMutex);

    free(group);
    freeRecievedMessage(&recievedMessage);
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
}

//username is the user who is sending message
void sendMessageUser(int current_user_socket, thread_arg* threadArg) {
    recieved_message recievedMessage;

    recievedMessage.arr = recvExactMsg(&recievedMessage.size_m, current_user_socket);
    
    recievedMessage.user_to_send = recvExactMsg(&recievedMessage.size_u, current_user_socket);

    writeToFileUser(&recievedMessage, threadArg->curr->username, recievedMessage.user_to_send, threadArg->user_fileMutex);

    pthread_mutex_lock(threadArg->mutex);
    user_info info = findUser(threadArg->user_Map, recievedMessage.user_to_send);
    pthread_mutex_unlock(threadArg->mutex);

    if(info.sockid == 0 || info.mutex == NULL) {
        printf("CANNOT FIND USER! \n");
        return;
    }

    strncpy(recievedMessage.user_to_send, threadArg->curr->username, strlen(threadArg->curr->username) + 1);
    recievedMessage.size_m = htonl(recievedMessage.size_m + 1);
    recievedMessage.size_u = htonl(recievedMessage.size_u + 1);

    pthread_mutex_lock(info.mutex);
    sendMessage(&recievedMessage, info.sockid, MSG_SEND);
    pthread_mutex_unlock(info.mutex);

    freeRecievedMessage(&recievedMessage);

    printf("Sent to the new client\n");
}

void freeRecievedMessage(recieved_message* recievedMessage) {
    free(recievedMessage->arr);
    free(recievedMessage->user_to_send);
}

void sendMessage(recieved_message* message_struct, int socket_id, int type_of_message) {
    send(socket_id, &type_of_message, sizeof(type_of_message), 0);
    send(socket_id, &(message_struct->size_m), sizeof(uint32_t), 0);
    send(socket_id, (message_struct->arr), ntohl(message_struct->size_m), 0);
    send(socket_id, &(message_struct->size_u), sizeof(uint32_t), 0);
    send(socket_id, (message_struct->user_to_send), ntohl(message_struct->size_u), 0);
}

void setupDir(char* username, char* base_path) {
    size_t len = strlen(base_path) + strlen(username) + 2;
    char* dir_location = malloc(len);
    snprintf(dir_location, len, "%s/%s", base_path, username);
    mkdir(dir_location, 0776);
    free(dir_location);
}

char* setupFileStringUser(char *username, char* username_to_send) {
    size_t len = strlen("logs/users/") + strlen(username) + 1 + strlen(username_to_send) + 4 + 1;
    char* file_location = malloc(len);
    snprintf(file_location, len, "logs/users/%s/%s.txt", username, username_to_send);
    return file_location;
}

char* setupFileStringUserFile(char *username, char* user_to_send, char* filename) {
    size_t len = strlen("logs/files/") + strlen(username) + strlen(user_to_send) + 2;
    char* file_location = malloc(len);
    snprintf(file_location, len, "logs/files/%s/%s", username, user_to_send);
    mkdir(file_location, 0777);
    char* file_location_new = realloc(file_location, len + strlen(filename) + 1);
    snprintf(file_location_new, len + strlen(filename) + 1, "logs/files/%s/%s/%s", username, user_to_send, filename);
    return file_location_new;
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

void initFileDataStructure(recievedFile* file, uint32_t file_size) {
    file->arr = malloc(file_size);
    memset(file->arr, 0, file_size);
}

uint32_t recvSize(int sockid) {
    uint32_t size = 0;
    recv(sockid, &size, sizeof(uint32_t), 0);
    return ntohl(size);
}

void processFile(recievedFile* file, uint32_t file_size) {
    file->user_to_send[49] = '\0';
    file->size_m = file_size;
    file->size_u = strlen(file->user_to_send) + 1;
    file->size_f_name = strlen(file->filename_to_send) + 1;
}

void saveFile(recievedFile* file, user* user) {
    char* path = setupFileStringUserFile(user->username, file->user_to_send, file->filename_to_send);
    FILE* fp = fopen(path, "w");
    fwrite(file->arr, 1, file->size_m, fp);
    fclose(fp);
}


void sendFileUser(thread_arg* arg) {
    recievedFile file;
    file.arr = recvExactMsg(&file.size_m, arg->curr->sockid);
    file.user_to_send = recvExactMsg(&file.size_u, arg->curr->sockid);
    file.filename_to_send = recvExactMsg(&file.size_f_name, arg->curr->sockid);
    saveFile(&file, arg->curr);
    insertFile(arg->user_Files, arg->curr, file.filename_to_send);
    sendFile(&file, arg);
    freeFile(&file);
}

void freeFile(recievedFile* file) {
    free(file->arr);
    free(file->filename_to_send);
    free(file->user_to_send);
}

void sendFileGroupMethod(recievedFile* file, thread_arg* threadArg) {
    int type_of_message = FILE_GROUP;

    pthread_mutex_lock(threadArg->mutex);

    for(int i = 0; i < MAXGROUPS; i++) {
        if(threadArg->user_Map->m_userArr[i] == NULL) {continue;}
        if(threadArg->user_Map->m_userArr[i]->sockid == threadArg->curr->sockid) {continue;}

        pthread_mutex_lock(threadArg->user_Map->m_userArr[i]->user_mutex);
        int sockid = threadArg->user_Map->m_userArr[i]->sockid;
        send(sockid, &type_of_message, sizeof(type_of_message), 0);
        sendUsername(threadArg->curr->username, strlen(threadArg->curr->username) + 1, sockid);
        sendUsername(file->filename_to_send, file->size_f_name, sockid);
        sendUsername(file->user_to_send, file->size_u + 1, sockid);
        pthread_mutex_unlock(threadArg->user_Map->m_userArr[i]->user_mutex);
    }
    pthread_mutex_unlock(threadArg->mutex);
}

void sendFileGroup(thread_arg* arg) {
    recievedFile file;

    file.arr = recvExactMsg(&file.size_m, arg->curr->sockid);
    file.user_to_send = recvExactMsg(&file.size_u, arg->curr->sockid);
    file.filename_to_send = recvExactMsg(&file.size_f_name, arg->curr->sockid);

    saveFile(&file, arg->curr);
    sendFileGroupMethod(&file, arg);
    freeFile(&file);
}

void handleRoomCreation(thread_arg* curr_user, int current_user_socket) {
    ChatRoom* newRoom = malloc(sizeof(ChatRoom));
    newRoom->ChatRoomName = recvExactMsg(&newRoom->name_length, current_user_socket);

    pthread_mutex_lock(curr_user->mutex);
    insert_ChatRoom(curr_user->ChatRoom_list, newRoom);
    pthread_mutex_unlock(curr_user->mutex);
                
    roomMethodCreation(curr_user, ROOM_CREATE, newRoom->ChatRoomName, strlen(newRoom->ChatRoomName) + 1);
}

int getFileSize(FILE* fp) {
    fseek(fp, 0, SEEK_END);
    int size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    return size;
}

void downloadFile(thread_arg* threadArg) {
    pthread_mutex_lock(threadArg->curr->user_mutex);

    uint32_t filename_size = 0;
    char* filename = recvExactMsg(&filename_size, threadArg->curr->sockid);
    filename[filename_size] = '\0';
    
    FILE* fp = fopen(filename, "r");
    if(fp == NULL) {return;}

    int size = getFileSize(fp);

    char* file_data = malloc(size);

    fread(file_data, 1, size, fp);

    int type_of_message = FILE_DOWNLOAD;
    send(threadArg->curr->sockid, &type_of_message, sizeof(type_of_message), 0);
    sendSize(size, threadArg->curr->sockid);
    sendAll(file_data, threadArg->curr->sockid, size);

    pthread_mutex_unlock(threadArg->curr->user_mutex);

    fclose(fp);
    free(filename);
}


void *createConnection(void *arg) {
    int n;
    uint32_t hdr;

    message_s_group *message_to_send_group = (message_s_group*) malloc(sizeof(message_s_group));

    thread_arg* curr_user = (thread_arg*)arg;

    setupDir(curr_user->curr->username, "logs/users");
    setupDir(curr_user->curr->username, "logs/files");

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
        else if(type == FILE_DOWNLOAD) {
            downloadFile(curr_user);
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