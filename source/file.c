#include <stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include <pthread.h>
#include "../header/file.h"
#include "../header/util.h"

File * filecreate(char* path, int fd)
{
  File *fnew=malloc(sizeof(File));
  fnew->fd=fd;
  fnew->isopen=0;
  fnew->lock=0;
  strcpy(fnew->path_name,path);
  pthread_mutex_init(&(fnew->mtxf),NULL);

 return fnew;
}
void OpenFile(File **f)
{
 
    (*f)->isopen=1;
 
}
void LockFile(File **f)
{
  
    (*f)->lock=1;
  
}
void UnlockFile(File** f)
{
  
    (*f)->lock=0;
 
}
void CloseFile(File** f)
{
  
    (*f)->isopen=0;
  
}
void appendFile(File** f,void* cont,int dim)
{
  
  (*f)->dim=dim;
  (*f)->cont=malloc((*f)->dim);
  memcpy((*f)->cont,cont,(*f)->dim);
 
}

