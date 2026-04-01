#include <stdio.h>
#include <stdlib.h>
#include "chat_room_list.h"

ChatRoomList* init_ChatRoom_list()
{
    ChatRoomList* ChatRoom_list = malloc(sizeof(ChatRoomList));
    ChatRoom_list->head = NULL;
    ChatRoom_list->tail = NULL;
    return ChatRoom_list;
}

void destructor_ChatRoom_list(ChatRoomList *ulist)
{
    ChatRoom *cur = ulist->head;
    while (cur) {
        ChatRoom *next = cur->next;
        free(cur->ChatRoomName);
        free(cur);             // free the ChatRoom struct
        cur = next;
    }
    ulist->head = NULL;
    free(ulist);
}

void insert_ChatRoom(ChatRoomList *a, ChatRoom *room)
{
    room->next = NULL;

    if (a->head == NULL) {
        a->head = room;
        a->tail = room;
    }
    else {
        a->tail->next = room;
        a->tail = room;
    }

    ++(a->size);
}

void remove_ChatRoom(ChatRoomList *a, ChatRoom *room)
{
    ChatRoom* temp = a->head;
    if (temp == room) {
        a->head = a->head->next;
        if (a->tail == temp) {
            a->tail = a->head;
        }
        free(temp);
        a->size--;
        return;
    }
    while (temp->next != NULL) {
        if (temp->next == room) {
            if (temp->next == a->tail) {
                a->tail = temp;
            }
            ChatRoom* temp_new = temp->next->next;
            free(temp->next);
            temp->next = temp_new;
            a->size--;
            return;
        }
        temp = temp->next;
    }
}
