
#include<string.h>
#define MAXSIZE 130
struct fil
{
   int isopen; // controlla se ol fle è stato già aperto
   char path_name[MAXSIZE]; //nome del file
   void* cont; 
   int dim; // dimensione del file 
   int fd; // client che ha aperto quel file
   int lock; //1 se c'è la lock su quel file
   pthread_mutex_t mtxf;
};
typedef struct fil File;
File * filecreate(char *name, int client);
void OpenFile(File** f);
void LockFile(File** f);
void UnlockFile(File** f);
void CloseFile(File** f);
void appendFile(File** f,void* cont, int dim);

