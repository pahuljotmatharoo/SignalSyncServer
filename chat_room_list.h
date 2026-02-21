#ifndef CHAT_ROOM_LIST_H
#define CHAT_ROOM_LIST_H
#define chatroom_length 50
#include "chat_room.h"

typedef struct ChatRoomList {
    ChatRoom *head;
    ChatRoom *tail;
    unsigned int size;
} ChatRoomList;

void init_ChatRoom_list(ChatRoomList *a);
void destructor_ChatRoom_list(ChatRoomList *ulist);
void insert_ChatRoom(ChatRoomList *a, ChatRoom* room);
void remove_ChatRoom(ChatRoomList *a, ChatRoom *room);

#endif