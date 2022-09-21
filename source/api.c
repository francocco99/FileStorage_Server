
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
#include "../header/Protcol.h"



int fd_skt;
// file descriptor del socket
int pidclient;
int stampe=0;

//funzione che restituisce il path assoluto dato il path relativo
void get_path(const char* pathname,char* result);

void  letturafile(char* dirname);

/* se il server non accetta immediatamente, la connessione viene ripetuta ogni msec
fino allo scadere del tempo msec*/
int openConnection(const char* sockname, int msec, const struct timespec abstime)
{
   errno=0;
   int result;
   //Setup Socket
   struct sockaddr_un sa;
   strncpy(sa.sun_path,sockname,strlen(sockname)+1);
   sa.sun_family=AF_UNIX;
   fd_skt=socket(AF_UNIX,SOCK_STREAM,0);
   if(fd_skt==-1)
   {
      perror("socket"); 
      return -1;
   }

   //tempo
   struct  timespec current;
   clock_gettime(CLOCK_REALTIME,&current);
   while((result=connect(fd_skt,(struct sockaddr*)&sa,sizeof(sa)))!=0)
   { 
      if(abstime.tv_sec<=current.tv_sec)
      {  
         printf("ERROR: connessione non riuscita timeout raggiunto\n"); 
         return -1;
      }
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
   int dim;
   int result;
   int filed;
   char namefile[MAXSIZE];
   request r;
   errno=0;

   get_path(pathname,namefile);
   strcpy(r.pathname,namefile);
   r.OP=OPEN;
   r.size=0;
   
   
   if(fd_skt< 0)
   {
      errno=EBADF;;
      perror("openFile");
      return -1;
   }
   if(flags!=O_OPEN && flags!=O_CREATE && flags!=O_LOCK)
   {
      errno=EINVAL;
      perror("openFile");
      return -1;
   }
   if((filed=open(pathname,O_RDONLY))==-1)
   {
      errno=ENOENT;
      printf("Errore: File non esistente\n");
      return -1;
   }
   if(filed!=-1){close(filed);}
   switch (flags)
   {
   case O_CREATE:
      r.flags=O_CREATE;
      writen(fd_skt,&r,sizeof(request)); 
      readn(fd_skt,&result,sizeof(int));
      switch (result)
      {
         case SUCCESS:
            if(stampe)
            {
               printf("File %s creato correttamente\n",r.pathname);
            }
            return 1;
            break;
         case YESCONTAIN:
            if(stampe)
            {
               printf("ERROR:File %s già presente\n",r.pathname);
            }
            return -1;
         break;
         case TOMUCHFILE:
            
            //lettura ° file rispedito indietro a causa di un capacity miss
            
            readn(fd_skt,&dim,sizeof(int)); 
            int len=0;
            readn(fd_skt,&len,sizeof(int));
            readn(fd_skt,namefile,len);
            namefile[len]='\0';
            char* cont=malloc(sizeof(char)*dim);
            readn(fd_skt,cont,dim);
            if(stampe)
            {
               printf("ERROR:Server pieno\n\n");
               printf("File inviato dal Server:\n\n");
               printf("DIMENSIONE DEL FILE %d\n",dim);
               printf("LUNGHEZZA DEL PATHNAME %d\n",len);
               printf("NOME DEL FILE:%s\n\n",namefile);
            }
            free(cont);
              
            return 1;
         break;
         default:
            break;
      }
   break;
   case O_CRLK :
      r.flags=O_CRLK ;
      writen(fd_skt,&r,sizeof(request)); 
      readn(fd_skt,&result,sizeof(int));
      switch (result)
      {
         case SUCCESS:
            if(stampe)
            {
               printf("File %s creato correttamente e lock avvenuta\n",r.pathname);
            }
            return 1;
            break;
         case YESCONTAIN:
            if(stampe)
            {
               printf("ERROR:File %s già presente\n",r.pathname);
            }
            return -1;
         break;
         case TOMUCHFILE:
            
            //lettura ° file rispedito indietro a causa di un capacity miss
            
            readn(fd_skt,&dim,sizeof(int)); 
            int len=0;
            readn(fd_skt,&len,sizeof(int));
            readn(fd_skt,namefile,len);
            namefile[len]='\0';
            char* cont=malloc(sizeof(char)*dim);
            readn(fd_skt,cont,dim);
            if(stampe)
            {
               printf("ERROR:Server pieno\n\n");
               printf("File inviato dal Server:\n\n");
               printf("DIMENSIONE DEL FILE %d\n",dim);
               printf("LUNGHEZZA DEL PATHNAME %d\n",len);
               printf("NOME DEL FILE:%s\n\n",namefile);
            }
            free(cont);
              
            return 1;
         break;
         default:
            break;
      }
   break;

   case O_LOCK:
    
      r.flags=O_LOCK;
      writen(fd_skt,&r,sizeof(request)); // invio il mess vero e proprio
      readn(fd_skt,&result,sizeof(int));
      switch (result)
      {
         case SUCCESS:
            if(stampe==1)
            {
               printf("File %s aperto correttamente con LOCK annessa\n",r.pathname);
            }
            return 1;
            break;
         case NOTPRESENT:
            if(stampe==1)
            {
               printf("ERROR:File %s non presente sul Server\n",r.pathname);
            }
            return -1;
         default:
         break;
      }

   break;

   case O_OPEN:
      r.flags=O_OPEN;
      writen(fd_skt,&r,sizeof(request)); // invio il mess vero e proprio
      readn(fd_skt,&result,sizeof(int));
      switch (result)
      {
         case SUCCESS:
            if(stampe==1)
            {
               printf("File %s aperto correttamente\n",r.pathname);
            }
            return 1;
            break;
         case NOTPRESENT:
            if(stampe==1)
            {
               printf("ERROR:File %s non presente sul Server\n",r.pathname);
            }
            return -1;
         default:
            break;
      }
   break;
   
   }
   return -1;
}
int closeConnection(const char* sockname)
{
   errno=0;
   request r;
   int result;
   if(fd_skt< 0)
   {
      errno=ENOTCONN;
      perror("closeConnection");
      return -1;
   }
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
            printf("Connessione chiusa con successo\n");
         }
         if(close(fd_skt)!=0)
         {
            printf("ERROR:nella chiusura della socket");
            return -1;
         }
            return 1;
      break;
    }
   return -1;
}

int readFile(const char* pathname, void** buf, size_t* size)
{
   errno=0;
   request r;
   int result;
   int dim;
   char* cont;
   char namefile[MAXSIZE];
   if(fd_skt< 0)
   {
      errno=EBADF;
      perror("readFile");
      return -1;
   }
   get_path(pathname,namefile);
   strcpy(r.pathname,namefile);
   r.OP=READ;
   r.size=0;
   r.flags=0;
   
   writen(fd_skt,&r,sizeof(r));
   readn(fd_skt,&result,sizeof(int));
   
   switch (result)
   {
      case SUCCESS:
         readn(fd_skt,&dim,sizeof(int));
         cont=malloc(sizeof(char)*dim);
         readn(fd_skt,cont,dim);
         *buf=cont;
         *size=dim;
         if(stampe==1)
         {   
            printf("File %s letto con successo\n",pathname);
         }
         
         return 1;
      break; 
      case NOTOPEN:
         if(stampe)
            printf("ERROR: impossibile aprire il file\n");
         return -1;
      break;

      case NOTPRESENT:
         if(stampe)
            printf("ERROR: read fallita file %s non presente\n", pathname);
         return -1;
      break;
      case NOTPERMISSION:
         if(stampe)
            printf("ERROR: l'utente non ha i permessi\n");
         return -1;
      break;
      
   }
   return -1;
} 

int readNFiles(int n, const char* dirname)
{
   errno=0;
   request r;
   int result;
   int number; //numero di file resituita dal server
   if(fd_skt< 0)
   {
      errno=EBADF;
      perror("readNFiles");
      return -1;
   }
   r.OP=READN;
   r.flags=n;
   r.size=0;
   r.fd=0;
   strcpy(r.pathname,"readN");
   writen(fd_skt,&r,sizeof(r));
   read(fd_skt,&result,sizeof(int));
   switch (result)
   {
      case SUCCESS:
         if(stampe)
         {
            if(n!=0)
               printf("Lettura di %d file:\n\n",n);
            else
               printf("Lettura di tutti i file presenti nel server\n\n");
         }
         
         readn(fd_skt,&number,sizeof(int));
         while(number>0)
         {
            letturafile((char*)dirname);
            number--;
         }
         
         return 1;
      break;
      case EMPTY:
         if(stampe==1)
         {
            printf("Server vuoto\n");
            
         }
         return -1;
      break;   
   }
   return -1;
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
   char namefile[MAXSIZE];
   if(fd_skt< 0)
   {
      errno=EBADF;
      perror("writeFile");
      return -1;
   }
   if(pathname==NULL)
   {
      errno=EINVAL; 
      return -1;
   }
   //RECUPERO IL FILE
   int filed;
   if((filed=open(pathname,O_RDONLY))==-1)
   {
      errno=ENOENT;
      printf("ERRORE: in apertura, file non esistente");
      return -1;
   }
   
   struct stat sb;
   if(stat(pathname, &sb) == -1) 
   {
      errno = EBADF;
      return -1;
   }
   buffer=malloc(sb.st_size);
   int dimfile=read(filed,buffer,sb.st_size);
   close(filed);
   /////
   r.OP=WRITE;
   r.flags=0;
   r.size=dimfile;
   get_path(pathname,namefile);
   strcpy(r.pathname,namefile);
   writen(fd_skt,&r,sizeof(r));
   writen(fd_skt,buffer,sb.st_size);
   
   readn(fd_skt,&result,sizeof(int));
   //Controllare se è  stato restituito un file o un codice di errore
   switch (result)
   {
      case SUCCESS:
         if(stampe==1)
         {
            printf("File %s inserito correttamente\n", r.pathname);
         }
         free(buffer);
         return 1;
         break;
      case NOTPRESENT:
         if(stampe==1)
         {
            printf("File %s non presente\n", r.pathname);
         }
         free(buffer);
         return -1;
      case NOTOPEN:
         if(stampe==1)
         {
            printf("ERROR:File %s non aperto\n",r.pathname);
         }
         free(buffer);
         return -1;
      case SERVERFULL:
         if(stampe==1)
         {
            printf("ERRORE: Server Pieno\n\n");
            printf("-------------FILE INVIATI DAL SERVER-------------\n\n");
         }
        
         
         //lettura ° file rispediti indietro
         readn(fd_skt,&n,sizeof(int));
         for(int i=0;i<n;i++)
         {
            readn(fd_skt,&dim,sizeof(int));

            if(stampe==1)
               printf("DIMENSIONE DEL FILE %d\n",dim);

            int len;
            readn(fd_skt,&len,sizeof(int));

            if(stampe==1)
               printf("LUNGHEZZA DEL PATHNAME %d\n",len);

            readn(fd_skt,namefile,len);
            namefile[len]='\0';

            if(stampe==1)
               printf("NOME DEL FILE:%s\n\n",namefile);

            char* cont=malloc(sizeof(char)*dim);
            readn(fd_skt,cont,dim);
            if(dirname!=NULL)
            {
               struct stat statbuf;

               if(stampe==1)
                  printf("Backup Folder:%s\n",dirname);

               if(stat(dirname, &statbuf)==-1) 
               {
                  if(stampe==1)
                     printf("Errore: Directory %s non esistente \n\n", dirname);  
                  errno=EBADF;
                  return -1;
                  
               }
               if(S_ISDIR(statbuf.st_mode)) 
               {
                  char* newdir=strc(dirname,"/");      
                  FILE* filed;
                  char *filename;
                  char** temp;    
                  int result=parse_str(&temp,namefile,"/");
                  filename=strc(newdir,temp[result-1]);
                  if((filed=fopen(filename,"wb"))==NULL)
                  {
                     perror("file.txt, in apertura");
                     errno=EBADF;
                     return -1;
                  }
                  else
                  {
                     if(fwrite(cont,sizeof(char),dim,filed)==0)
                     {
                        if(stampe==1)
                           printf("ERRORE NELLA SCRITTURA\n");
                     }
                     else
                     {
                        if(stampe==1)
                           printf("File %s scritto correttamente nella cartella %s\n\n",filename,dirname);
                     }
                  }
                  free(temp);
                  free(filename);
                  fclose(filed);
               }
            }
            free(cont);
         }
         if(stampe==1)
         {
            printf("-------------------------------------------------\n\n");
            printf("File %s inserito correttamente\n", pathname);
         }
         return 1;
      break;
      default:

      break;
   }
   free(buffer);
   return -1;
}

int appendToFile(const char* pathname, void* buf, size_t size, const char* dirname)
{
   int n;
   int dim;
   errno=0;
   request r;
   int result;
   char namefile[MAXSIZE];

   if(fd_skt< 0)
   {
      errno=EBADF;
      perror("appendToFile");
      return -1;
   }
   
   r.OP=APPEND;
   r.size=size;
   r.flags=0;
   get_path(pathname,namefile);
   strcpy(r.pathname,namefile);


   writen(fd_skt,&r,sizeof(r));
   ///write del contenuto
   writen(fd_skt,buf,size);
  
   
   readn(fd_skt,&result,sizeof(int));
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
            printf("ERROR:File %s non presente sul server\n",r.pathname);
         }
         return -1;
      case NOTOPEN:
         if(stampe==1)
         {
            printf("ERROR:File non aperto\n");
         }
         return -1;
      case SERVERFULL:
            
         readn(fd_skt,&n,sizeof(int));
         for(int i=0;i<n;i++)
         {
            
            readn(fd_skt,&dim,sizeof(int));

            if(stampe)
               printf("DIMENSIONE DEL FILE %d\n",dim);

            int len;
            readn(fd_skt,&len,sizeof(int));

            if(stampe)
               printf("LUNGHEZZA DEL PATHNAME %d\n",len);

            readn(fd_skt,namefile,len);
            namefile[len]='\0';
            
            if(stampe)
               printf("NOME DEL FILE:%s\n",namefile);

            char* cont=malloc(sizeof(char)*dim);
            readn(fd_skt,cont,dim);
   
            if(dirname!=NULL)
            {
               struct stat statbuf;
               printf("backup folder: %s\n",dirname);
               if(stat(dirname, &statbuf)==-1) 
               {
                  printf("Errore: directory %s non esistente\n", dirname);
                  errno=EBADF;
                  return -1;  
               }
               if(S_ISDIR(statbuf.st_mode)) 
               {      
                  char* newdir=strc(dirname,"/"); 
                  FILE* filed;
                  char *filename;
                  char** temp;    
                  int result=parse_str(&temp,namefile,"/");
                  filename=strc(newdir,temp[result-1]);
                  if((filed=fopen(filename,"wb"))==NULL)
                  {
                     perror("file.txt, in apertura");
                     errno=EBADF;
                     return -1;
                  }
                  else
                  {
                     if(fwrite(cont,sizeof(char),dim,filed)==0)
                     {
                        printf("ERRORE NELLA SCRITTURA\n");
                     }
                     else
                     {
                        printf("File %s scritto correttamente nella cartella\n",filename);
                     }
                  }
                  free(temp);
                  free(filename);
                  fclose(filed);
               }
            }
            free(cont);
         }
         return -1;
      break;
   }
   return -1;
}

int lockFile(const char* pathname)
{
   errno=0;
   request r;
   int result;
   char namefile[MAXSIZE];
   if(fd_skt< 0)
   {
      errno=EBADF;
      perror("lockFile");
      return -1;
   }
   get_path(pathname,namefile);
   strcpy(r.pathname,namefile);
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
            printf("la LOCK su %s è avenuta con successo\n",r.pathname);   
         }
         return 1;
      break;
      case NOTPRESENT:
         if(stampe==1)
         {
            printf("ERROR:File %s non presente sul server\n",r.pathname);
         }
         return -1;
      break;
      case NOTPERMISSION:
         if(stampe==1)
         {
            printf("ERROR:l'utente non ha i permessi per settare la lock sul file %s\n",r.pathname);
         }
         return -1;
      break;
      case NOTOPEN:
         if(stampe==1)
         {
            printf("ERROR:File %s non aperto\n",r.pathname);
         }
         return -1;
      break;

   }
   return -1;
}

int unlockFile(const char* pathname)
{
   errno=0;
   request r;
   int result;
   char namefile[MAXSIZE];
   if(fd_skt< 0)
   {
      errno=EBADF;
      perror("unlockFile");
      return -1;
   }
   get_path(pathname,namefile);
   strcpy(r.pathname,namefile);
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
            printf("la Lock sul file: '%s' è stata rilasciata  con successo\n",r.pathname);
         }
      return 1;
      break;
      case NOTPRESENT:
         if(stampe==1)
         {
            printf("ERROR: Operzione fallita, File: '%s' non presente\n",r.pathname);
         }
         return -1;
      break;
      case NOTPERMISSION:
         if(stampe==1)
         {
            printf("ERROR:  L'utente non può togliere la lock dal File %s\n",r.pathname);
         }
         return -1;
      break;
   }
   return -1;
}

int closeFile(const char* pathname)
{
   errno=0;
   request r;
   int result;
   char namefile[MAXSIZE];
   get_path(pathname,namefile);
   strcpy(r.pathname,namefile);
   if(fd_skt< 0)
   {
      errno=EBADF;
      perror("closeFile");
      return -1;
   }
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
            printf("Chiusura di %s avvenuta con successo\n",r.pathname);
         }
         return 1;
      break;
      case NOTPRESENT:
         if(stampe==1)
         {
              printf("ERROR:Chiusura fallita File %s non presente\n",r.pathname);
         }
         return -1;
      break;
      case NOTPERMISSION:
         if(stampe==1)
         {
            printf("ERROR:Chiusura fallita l'utente non può  chiudere questo file\n");
         }
         return -1;
      break;
   }
   return -1;
}

int removeFile(const char* pathname)
{
   request r;
   int result;
   char namefile[MAXSIZE];
   if(fd_skt< 0)
   {
      errno=EBADF;
      perror("removeFile");
      return -1;
   }
   get_path(pathname,namefile);
   strcpy(r.pathname,namefile);
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
            printf("Rimozione di %s avvenuta con successo\n",r.pathname);
         }
         return 1;
      break;
      case NOTPRESENT:
         if(stampe==1)
         {
            printf("ERROR:Rimozione fallita File %s non presente\n",r.pathname);
         }
         return -1;
      case NOTLOCKED:
         if(stampe==1)
         {
            printf("ERROR:Rimozione fallita il File %s non  è in stato LOCKED\n",r.pathname);
         }
         return -1;
      break;
      case NOTPERMISSION:
         if(stampe==1)
         {
             printf("ERROR:Rimozione fallita l'utente non può rimuovere questo file %s\n",r.pathname);
         }
         return -1;
      break;
   }
   return -1;
}
void abilitastampe()
{
   stampe=1;
}


void  letturafile(char* dirname)
{

   errno=0;
   int dim;
   char* namefile;
   readn(fd_skt,&dim,sizeof(int));

   if(stampe)
      printf("DIMENSIONE DEL FILE %d\n",dim);

   int len;
   readn(fd_skt,&len,sizeof(int));

   if(stampe)
      printf("LUNGHEZZA DEL PATHNAME %d\n",len);

   namefile=malloc(sizeof(char)*len+1);
   readn(fd_skt,namefile,len);

   if(stampe)
      printf("NOME DEL FILE:%s\n\n",namefile);

   void* cont=malloc(dim);
   readn(fd_skt,cont,dim);
   if(dirname!=NULL)
   {
      struct stat statbuf;
      
      if(stampe)
         printf("Download Folder:%s\n",dirname);

      if(stat(dirname, &statbuf)==-1) 
      {
         printf("Errore: Directory non esistente %s\n\n", dirname);  
         errno=EBADF;
      }
      if(S_ISDIR(statbuf.st_mode)) 
      {      

        char* newdir=strc(dirname,"/");
         FILE* filed;
         char *filename;
         char** temp; 
         int result=parse_str(&temp,namefile,"/");
         filename=strc(newdir,temp[result-1]);
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
               printf("File '%s' scritto correttamente nella cartella %s\n\n",filename,dirname);
            }
         }
         free(temp);
         free(filename);
         fclose(filed);
      }
   }
   free(cont);
}

void get_path(const char* pathname,char* result)
{
  
   realpath(pathname,result);

}

