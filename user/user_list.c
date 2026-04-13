#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include "user_list.h"

user_list* init_user_list() {
    user_list*  client_list = malloc(sizeof(user_list));
    client_list->head = NULL;
    client_list->tail = NULL;
    return client_list;
}

void destructor_user_list(user_list *ulist) {
    user *cur = ulist->head;
    while (cur) {
        user *next = cur->next;
        free(cur);             // free the user struct
        cur = next;
    }
    ulist->head = NULL;
    free(ulist);
}

void insert_user(user_list *a, user* client) {
    //this means that its the first node
    if(a->head == NULL) {
        a->head = client;
        a->tail = client;
    }
    //we insert to the end always
    else {
        a->tail->next = client;
        a->tail = client;
    }
    a->size++;
    return;
}

//just removes the client from the list of current clients when they disconnect
void remove_user(user_list *a, user* client) {
    user* temp = a->head;
    if (temp == client) {
        a->head = a->head->next;
        if (a->tail == temp) {
            a->tail = a->head;
        }
        free(temp);
        a->size--;
        return;
    }
    while (temp->next != NULL) {
        if (temp->next == client) {
            if (temp->next == a->tail) {
                a->tail = temp;
            }
            user* temp_new = temp->next->next;
            free(temp->next);
            temp->next = temp_new;
            a->size--;
            return;
        }
        temp = temp->next;
    }
}


void print_client_list(user_list *a) {
    user* temp = a->head;
    while(temp != NULL) {
        printf("%d ", temp->client.sin_addr.s_addr);
        temp = temp -> next;
    }
}

user_map* initUserMap() {
    user_map* t_map = malloc(sizeof(user_map));
    for(size_t i = 0; i < MAXUSERS; i++) {
        t_map->m_userArr[i] = NULL;
    }
    t_map->m_size = 0;
    return t_map;
}

void destroyUserMap(user_map* t_map) {
    for(size_t i = 0; i < MAXUSERS; i++) {
        if(t_map->m_userArr[i]) {
            pthread_mutex_destroy(t_map->m_userArr[i]->user_mutex);
            free(t_map->m_userArr[i]->user_mutex);
            free(t_map->m_userArr[i]);
        }
    }
    free(t_map);
}

size_t hash(char* username) {
    size_t len = strlen(username);
    size_t sum = 0;
    for(size_t i = 0; i < len; i++) {
        sum += (int)(username[i]);
    }
    return (sum % MAXUSERS);
}

void insertUser(user_map* t_map, user_files* t_files, user *client) {
    size_t index = hash(client->username);

    while(t_map->m_userArr[index] != NULL) {
        index++;
    }

    t_map->m_userArr[index] = client;
    t_files->m_users[index] = client;
    t_map->m_size++;
}

void removeUser(user_map* t_map, user* client) {
    size_t index = hash(client->username);

    if(t_map->m_userArr[index] == NULL) {return;}

    while(t_map->m_userArr[index] != NULL) {
        if(strcmp(t_map->m_userArr[index]->username, client->username) == 0) {
            free(t_map->m_userArr[index]);
            t_map->m_userArr[index] = NULL;
            t_map->m_size--;
        }
        else {
            index++;
        }
    }

    if(t_map->m_userArr[index] == NULL) {return;}
}

user_info findUser(user_map* t_map, char* username) {
    size_t index = hash(username);
    user_info info;
    info.sockid = 0;
    info.mutex = NULL;

    if(t_map->m_userArr[index] == NULL) {return info;}

    while(t_map->m_userArr[index] != NULL) {
        if(strcmp(t_map->m_userArr[index]->username, username) == 0) {
            info.sockid = t_map->m_userArr[index]->sockid;
            info.mutex = t_map->m_userArr[index]->user_mutex;
            return info;
        }
        else {
            index++;
        }
    }
    return info;
}

char** resizeArray(char** arr, unsigned int* size) {
    arr = realloc(arr, *size * 2);
    *size = *size * 2;
    return arr;
}

user_files* initUserFilesMap() {
    user_files* files_map = (user_files*)malloc(sizeof(user_files));
    for(int i = 0; i < MAXUSERS; i++) {
        files_map->m_size[i] = 1;
    }
    for(int i = 0; i < MAXUSERS; i++) {
        files_map->m_capacity[i] = 1;
    }
    for(int i = 0; i < MAXUSERS; i++) {
        files_map->m_files[i] = NULL;
    }
}

void insertFile(user_files* t_map, user* client, char* file) {
    size_t index = hash(client->username);

    while(t_map->m_users[index] != NULL && t_map->m_users[index] != client) {
        index++;
    }

    if(t_map->m_size[index] == t_map->m_capacity[index]) {
        t_map->m_files[index] = resizeArray(t_map->m_files[index], &t_map->m_capacity[index]);
    }
    t_map->m_files[index][t_map->m_size[index]] = malloc(strlen(file) + 1);
    strcpy(t_map->m_files[index][t_map->m_size[index]], file);
    t_map->m_files[index][t_map->m_size[index]][strlen(file)] = '\0';
    t_map->m_size[index]++;
}
