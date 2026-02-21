#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include "user_list.h"

void init_user_list(user_list *a) {
    a->head = NULL;
    a->tail = NULL;
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

void initUserMap(user_map* t_map) {
    for(size_t i = 0; i < MAXUSERS; i++) {
        t_map->m_userArr[i] = NULL;
    }
    t_map->m_size = 0;
}

void destroyUserMap(user_map* t_map) {
    for(size_t i = 0; i < MAXUSERS; i++) {
        if(t_map->m_userArr[i]) {
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

void insertUser(user_map *t_map, user *client) {
    size_t index = hash(client->username);

    while(t_map->m_userArr[index] != NULL) {
        index++;
    }

    t_map->m_userArr[index] = client;
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

size_t findUser(user_map* t_map, char* username) {
    size_t index = hash(username);

    if(t_map->m_userArr[index] == NULL) {return -1;}

    while(t_map->m_userArr[index] != NULL) {
        if(strcmp(t_map->m_userArr[index]->username, username) == 0) {
            return index;
        }
        else {
            index++;
        }
    }

    return -1;
}
