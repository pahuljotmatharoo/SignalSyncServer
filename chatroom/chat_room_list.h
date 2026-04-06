#ifndef CHAT_ROOM_LIST_H
#define CHAT_ROOM_LIST_H

#define chatroom_length 50
#include "chat_room.h"

typedef struct ChatRoomList {
    ChatRoom* head;
    ChatRoom* tail;
    int       size;
} ChatRoomList;


ChatRoomList* init_ChatRoom_list();
void destructor_ChatRoom_list(ChatRoomList *ulist);
void insert_ChatRoom(ChatRoomList *a, ChatRoom* room);
void remove_ChatRoom(ChatRoomList *a, ChatRoom *room);

#endif