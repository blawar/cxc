#include <unistd.h>
#include <malloc.h>
#include <string.h>
#include <pthread.h>

#include <sys/types.h>
#ifndef WINDOWS
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#else
#include <WinSock2.h>
#endif

#define bool unsigned char
#define byte unsigned char
#define int8 unsigned char
#define int16 unsigned short
#define int32 unsigned int
#define int64 unsigned long
#define functor unsigned long

typedef long (*ANON_FUNC_TYPE)(void*, ...);


