#ifndef COMDEF_H
#define COMDEF_H

#include <string.h>

typedef signed char s8;
typedef signed short s16;
typedef signed int s32;
typedef signed long long int s64;

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long int u64;

#define NONE	0
#define FALSE	0
#define TRUE	1
#define CLEAR(x) memset(&(x), 0, sizeof(x))
#define UNUSED(expr) do{(void)(expr);}while(0)

#endif
