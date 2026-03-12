// messages.c
#include <stdio.h>
#include "messages.h"
void print_data(message_s *a) {
    for (int i = 0; i < 128; i++) {
        putchar(a->arr[i]);
        putchar(' ');
    }
}