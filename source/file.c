#include <stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include "../header/file.h"

File * filecreate(char* path, int fd)
{
  File *fnew=malloc(sizeof(File));
  fnew->fd=fd;
  fnew->isopen=0;
  fnew->lock=0;
  strcpy(fnew->path_name,path);


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

