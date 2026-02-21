#ifndef CHAT_ROOM_H
#define CHAT_ROOM_H

#define chatroom_length 50
#include "user_list.h"

typedef struct ChatRoom {
    char ChatRoomName[chatroom_length];
    struct ChatRoom* next;
} ChatRoom;

#endif