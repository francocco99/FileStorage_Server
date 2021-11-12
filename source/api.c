
#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#include <sys/stat.h>
#include <fcntl.h>
#include "../header/stringutil.h"
#include "../header/file.h"
#include "../header/api.h"

#define N 50
#define SOCKNAME "/mysock"

int fd_skt;
int pidclient;
int stampe=0;
void  letturafile(char* dirname);
/* se il server non accetta immediatamente, la connessione viene ripetuta ogni msec
fino allo scadere del tempo msec*/
int openConnection(const char* sockname, int msec, const struct timespec abstime)
{
   //socket association
   errno=0;
   int result;
   struct sockaddr_un sa;
   strncpy(sa.sun_path,SOCKNAME,strlen(SOCKNAME)+1);
   sa.sun_family=AF_UNIX;
   fd_skt=socket(AF_UNIX,SOCK_STREAM,0);
   if(fd_skt==-1){perror("socket"); exit(EXIT_FAILURE);}
   //tempo
   struct  timespec current;
   clock_gettime(CLOCK_REALTIME,&current);
   while((result=connect(fd_skt,(struct sockaddr*)&sa,sizeof(sa)))!=0)
   {
       
      if(abstime.tv_sec<=current.tv_sec){printf("ERROR:connection refused timeout raggiunto\n"); return -1;}
      usleep(1000*msec);
      clock_gettime(CLOCK_REALTIME,&current);
     
   }
   if(result!=-1)
   {
      return 1;
   }
   errno=CONNECTION_TIMEOUT;
   return -1;
}

int openFile(const char* pathname, int flags)
{
   errno=0;
   char *buff=malloc(sizeof(char)*N);
   request r;
   r.OP=OPEN;
   r.size=0;
   strcpy(r.pathname,pathname);
   int result;
  
   switch (flags)
   {
   case O_CREATE:
   
      r.flags=O_CREATE;
      writen(fd_skt,&r,sizeof(request)); // invio il mess vero e proprio
      readn(fd_skt,&result,sizeof(int));
      switch (result)
      {
         case SUCCESS:
            if(stampe)
            {
               printf("File creato correttamente\n");
            }
            return 1;
            break;
         case YESCONTAIN:
            if(stampe)
            {
               printf("ERROR:File già presente\n");
            }
            return -1;
         break;
         case TOMUCHFILE:
            if(stampe)
            {
                // ricevere un file da espellere
            }
         default:
            break;
      }

   break;

   case O_LOCK:
    
      r.flags=O_LOCK;
      write(fd_skt,&r,sizeof(request)); // invio il mess vero e proprio
      read(fd_skt,&result,sizeof(int));
       switch (result)
      {
         case SUCCESS:
            if(stampe==1)
            {
               printf("FIle aperto correttamente con LOCK annessa");
            }
            return 1;
            break;
         case NOTPRESENT:
            if(stampe==1)
            {
               printf("ERROR:file non presente sul Server");
            }
            return -1;
         default:
            break;
      }

   break;

   case 0:
      r.flags=0;
      write(fd_skt,&r,sizeof(request)); // invio il mess vero e proprio
      read(fd_skt,&result,sizeof(int));
      switch (result)
      {
         case SUCCESS:
            if(stampe==1)
            {
               printf("File aperto correttamente ");
            }
            return 1;
            break;
         case NOTPRESENT:
            if(stampe==1)
            {
               printf("ERROR:file non presente sul Server");
            }
            return -1;
         default:
            break;
      }
   break;
   
   }
   free(buff);
}
int closeConnection(const char* sockname)
{
   errno=0;
   request r;
   int result;
   r.OP=CLOSECONNECTION;
   r.flags=0;
   r.size=0;
   writen(fd_skt,&r,sizeof(r));
   readn(fd_skt,&result,sizeof(int));
   switch (result)
   {
      case SUCCESS:
         if(stampe==1)
         {
            printf("CONNECTION CLOSED\n");
            if(close(fd_skt)!=0)
            {
                printf("ERROR:nella chiusura della socket");
               return -1;
            }
            return 1;
          }
      break;
    }

}

int readFile(const char* pathname, void** buf, size_t* size)
{
   errno=0;
   request r;
   int result;
   int dim;
   char* cont;
   
   strcpy(r.pathname,pathname);
   r.OP=READ;
   r.size=0;
   r.flags=0;
   
   writen(fd_skt,&r,sizeof(r));
   printf("RICHIESTA INVIATA\n");
   readn(fd_skt,&result,sizeof(int));
   
   switch (result)
   {
      case SUCCESS:
         if(stampe==1)
         {
              
               readn(fd_skt,&dim,sizeof(int));
               printf("HO LETTO LA DIMENSIONE DEL FILE %d\n",dim);
               cont=malloc(sizeof(char)*dim);
               readn(fd_skt,cont,dim);
               printf("HO LETTO IL CONTENUTO:%s",cont);
               *buf=cont;
               *size=dim;
         }
         return 1;
      break; 
      case NOTOPEN:
         printf("ERROR: impssibile aprire il file\n");
         return -1;
      break;

      case NOTPRESENT:
         printf("ERROR: file non presente\n");
         return -1;
      break;
      case TOMUCHFILE:

      break;
      
   }
}                                        
int readNFiles(int n, const char* dirname)
{
   errno=0;
   request r;
   int result;
   int number; //numero di file resituita dal server

   // per leggere i file dal server
   char namefile[N];
   char* cont;
   int dim;
   
   r.OP=READN;
   r.flags=n;
   r.size=0;
   strcpy(r.pathname,"readN");
   write(fd_skt,&r,sizeof(r));
   read(fd_skt,&result,sizeof(int));
   switch (result)
   {
      case SUCCESS:

         if(stampe==1)
         {
            printf("Succes:ReadN si può fare\n");
            readn(fd_skt,&number,sizeof(int));
            printf("number:%d\n",number);
            while(number>0)
            {
               letturafile(dirname);
               number--;

            }
            return 1;
         }
      break;
      case EMPTY:
         if(stampe==1)
         {
            printf("HashMap vuota\n");
            return -1;
         }
      break;

      
      
   }
   
}
int writeFile(const char* pathname, const char* dirname)
{
   errno=0;
   request r;
   int result;
   void* buffer;
   //variabili in caso di capacity miss
   int n;
   int dim;
   char namefile[N];
   printf("qua ci arrivo\n");
   //RECUPERO IL FILE
    int filed;
   if((filed=open(pathname,O_RDONLY))==-1)
   {
      perror("ERRORE: in apertura");
      exit(EXIT_FAILURE);
   }
   
   struct stat sb;
   if(stat(pathname, &sb) == -1) { // impossibile aprire statistiche file
      errno = EBADF;
      return -1;
   }
   buffer=malloc(sb.st_size);
   int dimfile=read(filed,buffer,sb.st_size);
   /////
   r.OP=WRITE;
   r.flags=0;
   r.size=dimfile;
   strcpy(r.pathname,pathname);
   int k=writen(fd_skt,&r,sizeof(r));
   writen(fd_skt,buffer,sb.st_size);
   printf("CONTENUTO FILE DA INVIARE:%s",buffer);
   //CONTROLLARE SE è  stato restituito un file o un codice di errore
   
   readn(fd_skt,&result,sizeof(int));
   
  
   switch (result)
   {
      case SUCCESS:
         if(stampe==1)
         {
               printf("File inserito correttamente\n");
         }
         return 1;
         break;
      case NOTPRESENT:
         if(stampe==1)
         {
               printf("File non presente\n");
         }
         return -1;
      case NOTOPEN:
         if(stampe==1)
         {
            printf("ERROR:File non aperto\n");
         }
      case SERVERFULL:
         if(stampe==1)
         {
            printf("SERVER PIENO\n");
         }
         
         //lettura dimensione
         readn(fd_skt,&n,sizeof(int));
         for(int i=0;i<n;i++)
         {
            readn(fd_skt,&dim,sizeof(int));
            printf("HO LETTO LA DIMENSIONE DEL FILE %d\n",dim);
            int len;
            readn(fd_skt,&len,sizeof(int));
            printf("LUNGHEZZA DEL PATHNAME %d\n",len);
            readn(fd_skt,namefile,len);
            namefile[len]='\0';
            printf("HO LETTO IL NOME DEL FILE:%s\n",namefile);
            char* cont=malloc(sizeof(char)*dim);
            readn(fd_skt,cont,dim);
            printf("HO LETTO IL CONTENUTO:%s",cont);
            if(dirname!=NULL)
            {
               struct stat statbuf;
               printf("CARTELLA DOVE VADO A SCRIVERE IL FILE%s\n",dirname);
               if(stat(dirname, &statbuf)==-1) 
               {
                  perror("eseguendo la stat");
                  printf("Errore nel file %s\n", dirname);
                  
               }
               if(S_ISDIR(statbuf.st_mode)) 
               {      
                  FILE* filed;
                  char *filename;
                  char** temp;    
                  int result=parse_str(&temp,namefile,"/");
                  filename=strc(dirname,temp[result-1]);
                  if((filed=fopen(filename,"wb"))==NULL)
                  {
                     perror("file.txt, in apertura");
                     exit(EXIT_FAILURE);
                  }
                  else
                  {
                     if(fwrite(cont,sizeof(char),dim,filed)==0)
                     {
                        printf("ERRORE NELLA SCRITTURA\n");
                     }
                     else
                     {
                        printf("File scritto correttamente nella cartella\n");
                     }
                  }
                  free(temp);
                  free(filename);
                  fclose(filed);
               }
            }
            free(cont);
         }
      break;
      default:

       break;
   }
   free(buffer);
}
int appendToFile(const char* pathname, void* buf, size_t size, const char* dirname)
{
   int n;
         int dim;
   char namefile[N];
   errno=0;
   request r;
   int result;
   strcpy(r.pathname,pathname);
   r.OP=APPEND;
   r.size=size;
   r.flags=0;
   strcpy(r.pathname,pathname);
   write(fd_skt,&r,sizeof(r));

   writen(fd_skt,buf,size);
  
   printf("DENTRO APPEND\n");
   ///write del contenuto
   read(fd_skt,&result,sizeof(int));
   switch (result)
   {
      case SUCCESS:
         if(stampe==1)
         {
            printf("append avvenuta con successo\n");   
         }
         return 1;
         break;
      case NOTPRESENT:
         if(stampe==1)
         {
            printf("ERROR:File non presente sul server\n");
         }
         return -1;
      case NOTOPEN:
         if(stampe==1)
         {
            printf("ERROR:File non aperto\n");
         }
         return -1;
      case SERVERFULL:
         if(stampe==1)
         {
            
            readn(fd_skt,&n,sizeof(int));
         for(int i=0;i<n;i++)
         {
            
            readn(fd_skt,&dim,sizeof(int));
            printf("HO LETTO LA DIMENSIONE DEL FILE %d\n",dim);
            int len;
            readn(fd_skt,&len,sizeof(int));
            printf("LUNGHEZZA DEL PATHNAME %d\n",len);
            readn(fd_skt,namefile,len);
            namefile[len]='\0';
            printf("HO LETTO IL NOME DEL FILE:%s\n",namefile);
            char* cont=malloc(sizeof(char)*dim);
            readn(fd_skt,cont,dim);
            printf("HO LETTO IL CONTENUTO:%s",cont);
            if(dirname!=NULL)
            {
               struct stat statbuf;
               printf("CARTELLA DOVE VADO A SCRIVERE IL FILE%s\n",dirname);
               if(stat(dirname, &statbuf)==-1) 
               {
                  perror("eseguendo la stat");
                  printf("Errore nel file %s\n", dirname);
                  
               }
               if(S_ISDIR(statbuf.st_mode)) 
               {      
                  FILE* filed;
                  char *filename;
                  char** temp;    
                  int result=parse_str(&temp,namefile,"/");
                  filename=strc(dirname,temp[result-1]);
                  if((filed=fopen(filename,"wb"))==NULL)
                  {
                     perror("file.txt, in apertura");
                     exit(EXIT_FAILURE);
                  }
                  else
                  {
                     if(fwrite(cont,sizeof(char),dim,filed)==0)
                     {
                        printf("ERRORE NELLA SCRITTURA\n");
                     }
                     else
                     {
                        printf("File scritto correttamente nella cartella\n");
                     }
                  }
                  free(temp);
                  free(filename);
                  fclose(filed);
               }
            }
            free(cont);
         }
            //DAGESTIRE LAVORI IN corsooooooooooooooooooooooooooooooooo

         }
         return -1;

      break;
   }
   return ;
}

int lockFile(const char* pathname)
{
   errno=0;
   request r;
   int result;
   strcpy(r.pathname,pathname);
   r.OP=LOCKS;
   r.size=0;
   r.flags=0;
   write(fd_skt,&r,sizeof(r));
   read(fd_skt,&result,sizeof(int));
   switch (result)
   {
      case SUCCESS:
         if(stampe==1)
         {
            printf("la LOCK è avenuta con successo\n");   
         }
         return 1;
         break;
      case NOTPRESENT:
         if(stampe==1)
         {
            printf("ERROR:File non presente sul server\n");
         }
         return -1;

   }
}

int unlockFile(const char* pathname)
{
   errno=0;
   request r;
   int result;
   strcpy(r.pathname,pathname);
   r.OP=UNLOCKS;
   r.size=0;
   r.flags=0;
   write(fd_skt,&r,sizeof(r));
   read(fd_skt,&result,sizeof(int));
   switch (result)
   {
      case SUCCESS:
         if(stampe==1)
         {
            printf("la Lock è stata rilasciata  con successo\n");
         }
      return 1;
         break;
      case NOTPRESENT:
         if(stampe==1)
         {
            printf("ERROR: Operzione fallita File non presente\n");
         }
         return -1;
      break;
      case NOTPERMISSION:
         if(stampe==1)
         {
            printf("ERROR:  L'utente  non può togliere la lock da  questo file\n");
         }
         return -1;
      break;

   }
}

int closeFile(const char* pathname)
{
   errno=0;
   request r;
   int result;
   strcpy(r.pathname,pathname);
   r.OP=CLOSE;
   r.flags=0;
   r.size=0;
   write(fd_skt,&r,sizeof(r));
   read(fd_skt,&result,sizeof(int));
   switch (result)
   {
      case SUCCESS:
         if(stampe==1)
         {
            printf("Chiusura avvenuta con successo\n");
         }
         return 1;
      break;
      case NOTPRESENT:
         if(stampe==1)
         {
              printf("ERROR:Chiusura fallita File non presente\n");
         }
         return -1;
      break;
      case NOTPERMISSION:
         if(stampe==1)
         {
            printf("ERROR:Chiusura fallita L'utente non può eliminare questo file\n");
         }
         return -1;
      break;
   }

}
int removeFile(const char* pathname)
{
     request r;
   int result;
   strcpy(r.pathname,pathname);
   r.OP=REMOVE;
   r.flags=0;
   r.size=0;
   write(fd_skt,&r,sizeof(r));
   read(fd_skt,&result,sizeof(int));
   switch (result)
   {
      case SUCCESS:
         if(stampe==1)
         {
            printf("Rimozione avvenuta con successo\n");
         }
         return 1;
      break;
      case NOTPRESENT:
         if(stampe==1)
         {
            printf("ERROR:Rimozione fallita File non presente\n");
         }
         return -1;
      case NOTLOCKED:
         if(stampe==1)
         {
            printf("ERROR:il FIle non è stato in LOCKED\n");
         }
         return -1;
      break;
      case NOTPERMISSION:
         if(stampe==1)
         {
             printf("ERROR:Rimozione fallita L'utente non può rimuovere questo file\n");
         }
         return -1;
      break;
      
   }
}
void abilitastampe()
{
   stampe=1;
}
static inline int readn(long fd, void *buf, size_t size) {
    size_t left = size;
    int r;
    char *bufptr = (char*)buf;
    while(left>0) {
	if ((r=read((int)fd ,bufptr,left)) == -1) {
	    if (errno == EINTR) continue;
	    return -1;
	}
	if (r == 0) return 0;   // EOF
        left    -= r;
	bufptr  += r;
    }
    return size;
}

/** Evita scritture parziali
 *
 *   \retval -1   errore (errno settato)
 *   \retval  0   se durante la scrittura la write ritorna 0
 *   \retval  1   se la scrittura termina con successo
 */
static inline int writen(long fd, void *buf, size_t size) {
    size_t left = size;
    int r;
    char *bufptr = (char*)buf;
    while(left>0) {
	if ((r=write((int)fd ,bufptr,left)) == -1) {
	    if (errno == EINTR) continue;
	    return -1;
	}
	if (r == 0) return 0;  
        left    -= r;
	bufptr  += r;
    }
    return 1;
}

void  letturafile(char* dirname)
{
   errno=0;
   int dim;
   char* namefile;
   readn(fd_skt,&dim,sizeof(int));
   printf("HO LETTO LA DIMENSIONE DEL FILE %d\n",dim);
   int len;
   readn(fd_skt,&len,sizeof(int));
   printf("LUNGHEZZA DEL PATHNAME %d\n",len);
   readn(fd_skt,namefile,len);
   namefile[len]='\0';
   printf("HO LETTO IL NOME DEL FILE:%s\n",namefile);
   void* cont=malloc(dim);
   readn(fd_skt,cont,dim);
   printf("HO LETTO IL CONTENUTO:%s",cont);
   if(dirname!=NULL)
   {
      struct stat statbuf;
      printf("CARTELLA DOVE VADO A SCRIVERE IL FILE%s\n",dirname);
      if(stat(dirname, &statbuf)==-1) 
      {
         perror("eseguendo la stat");
         printf("Errore nel file %s\n", dirname);
         
      }
      if(S_ISDIR(statbuf.st_mode)) 
      {      
         FILE* filed;
         char *filename;
         char** temp;    
         int result=parse_str(&temp,namefile,"/");
         filename=strc(dirname,temp[result-1]);
         if((filed=fopen(filename,"wb"))==NULL)
         {
            perror("file.txt, in apertura");
            exit(EXIT_FAILURE);
         }
         else
         {
            if(fwrite(cont,sizeof(char),dim,filed)==0)
            {
               printf("ERRORE NELLA SCRITTURA\n");
            }
            else
            {
               printf("File scritto correttamente nella cartella\n");
            }
         }
         free(temp);
         free(filename);
         fclose(filed);
      }

   }
   free(cont);
}


