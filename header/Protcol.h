#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#define MAXS 130
#define OPEN   1 
#define LOCKS   2
#define UNLOCKS 3
#define WRITE  4
#define READ   5
#define CLOSE  6
#define REMOVE 7
#define APPEND 8
#define READN 9
#define CLOSECONNECTION 10
#define O_CREATE 11
#define O_LOCK   12 
#define O_CRLK   13
#define O_OPEN 14

#define SUCCESS 100
#define NOTPRESENT 200
#define YESCONTAIN 201
#define NOTOPEN 202
#define NOTLOCKED 203
#define NOTPERMISSION 204
#define TOMUCHFILE 205
#define SERVERFULL 206
#define EMPTY 207
#define CONNECTION_TIMEOUT 208

int writen(long fd, void *buf, size_t size);
int readn(long fd, void *buf, size_t size);

typedef struct request
{
   char pathname[MAXS];
   int size;
   int flags;
   int fd;
   int OP;
   void* contenuto;
}request;
