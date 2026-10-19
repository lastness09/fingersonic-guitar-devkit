#pragma once
#include <stddef.h>
extern "C" {
void* memcpy(void*,const void*,size_t);
void* memset(void*,int,size_t);
size_t strlen(const char*);
}
