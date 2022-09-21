
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include "../header/file.h"
#include "../header/stringutil.h"
#include "../header/util.h"
#include "../header/hash.h"
#include "../header/Protcol.h"


#define UNIX_PATH_MAX 108


static void* worker();
static void* dispatcher();

void configura(char* nomefile);
request * readMessage(int client);   /// metodo per la lettura dei messaggi
void sendMessage(int client, int message); // metodo per la scrittura di messagi sulla socket
void sendFile(int client,char* cont,int dim); // per mandare i messaggi
void readFiles(int fd,int n);// legge n file dal server
void Handlercapacity(int fd);//funzione per la gestione di capacity miss
void LOG_file(char* log);
void LOG(char* log, int info);
void freeStruct();
//rilascia le lock sui file di quel fd
void relaselock(int fd);
static void* signalHandler();

typedef struct serverconf
{
   char* socketname; // nome della socket
   char* filelog;
   int maxf; // numero massimo di file
   int space; // spazio massimo
   int nwork; // numero worker da predere dal file di config
}ServerConf;

ServerConf sc;


pthread_mutex_t mtxrequest = PTHREAD_MUTEX_INITIALIZER; 
pthread_cond_t condrequest = PTHREAD_COND_INITIALIZER; 

pthread_mutex_t mtxhash = PTHREAD_MUTEX_INITIALIZER;


pthread_mutex_t mtxlog = PTHREAD_MUTEX_INITIALIZER;



pthread_mutex_t mtxfifo= PTHREAD_MUTEX_INITIALIZER;



pthread_mutex_t mtxcon= PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condcon=PTHREAD_COND_INITIALIZER;

int pipefd[2];//pipe utilizzata dai worker per segnalare il completamento di una richiesta
int pipedisp[2]; //pipe utilizzata dal signal handler



// terminazione worker con segnali
int terminate=1; 
int gestore=1;
int closesighup=1;

int threadid=0;
int fd; //indice per scorrere  i fd
int fd_skt; // scoket di connessione
int fd_c;  
int fd_num=0; // max fd
int activecon=0;

fd_set set, // tutti i file descriptor attivi
   rdset;   // insieme di file pronti per la lettura

FILE* logfd=NULL; // filedescriptor log file

hash* Hash; //hashmap contenete i file
Lis fifo;   //coda per la gestione fifo dei file da rinviare
Lis queueric; //coda per la gestione delle richieste
int actual_size=0;

pthread_t disp;
pthread_t Hand;
pthread_t* threadp;
int main(int argc,char* argv[])
{
   
   configura(argv[1]);
   Hash=h_create(sc.maxf); //creazione tabella hash
   fifo=create();
   queueric=create();
      
   unlink(sc.socketname);
   //creo la pipe
   pipe(pipefd);
   pipe(pipedisp);

   ////SETUP SERVER
   struct sockaddr_un sa;
   strncpy(sa.sun_path,sc.socketname,UNIX_PATH_MAX);
   sa.sun_family=AF_UNIX;
   fd_skt=socket(AF_UNIX,SOCK_STREAM,0);
   bind(fd_skt,(struct sockaddr *)&sa,sizeof(sa));
   // MASCHERO I SEGNALI
   sigset_t mask;
   sigfillset(&mask);
   pthread_sigmask(SIG_SETMASK, &mask, NULL);
   

   // ATTIVAZIONE THREADPOOL
   threadp=malloc(sizeof(pthread_t)*sc.nwork);
   for (size_t i = 0; i < sc.nwork; i++)
   {
      pthread_create(&threadp[i],NULL,&worker,NULL);
   }
   // attivazione thread dispatcher e thread handler
   pthread_create(&Hand,NULL,&signalHandler,NULL);
   pthread_create(&disp,NULL,&dispatcher,NULL);
   pthread_join(Hand,NULL);
   printf("Server terminato\n");
   return 0;
}


// si occupa di attivare i thread worker che gestiranno le richieste 
static void* dispatcher()
{
   listen(fd_skt,SOMAXCONN);

  
   FD_ZERO(&set);
  
   FD_SET(fd_skt,&set);
   if (fd_skt > fd_num) fd_num = fd_skt;
  
   FD_SET(pipefd[0],&set); // pipe utilizzata dal worker per communicare con il dispatcher
   if (pipefd[0] > fd_num) fd_num = pipefd[0]; 
  
   FD_SET(pipedisp[0],&set); // pipe utilizzata dal thread signalhandler per communicare con il dispatcher
   if (pipedisp[0]> fd_num) fd_num=pipedisp[0]; 
  
   while(gestore)
   {
      rdset=set; // inizializzo ogni volta dato che la select modifica wrset
      
      if(select(fd_num+1,&rdset,NULL,NULL,NULL)==-1)
      {
         //caso errore
        perror("ERROR");
        return 0;
      }
      else
      {   
         for(fd=0;fd<=fd_num;fd++)
         { 
            if(FD_ISSET(fd,&rdset)) 
            {
               if(fd==fd_skt && closesighup) // PRIMA VOLTA CHE IL CLIENT SI CONNETTE
               {
                  fd_c=accept(fd_skt,NULL,0);  
                  LOCK(&mtxcon);
                     activecon++;
                  UNLOCK(&mtxcon);
                  LOG("Numero di connessioni attive ",0);
                  LOG("NC ",activecon);
                  FD_SET(fd_c,&set);
                  if (fd_c > fd_num){fd_num = fd_c;}
                  break;
               }
               else if(fd==pipefd[0])//il worker segnala che ha finito di gestire una richiesta
               {
                  int fd_o;
                  readn(pipefd[0],&fd_o,sizeof(int));
                  FD_SET(fd_o,&set);
                  break;
               }
               else if(fd==pipedisp[0]) //il signal handler lo utilizza all'arrivo di un segnale
               {
                  gestore=0;
               }
               else 
               {
                  FD_CLR(fd,&set);
                  request *r;
                  r=readMessage(fd);            
                  LOCK(&mtxrequest);
                  if(r!=NULL)
                  {
                     insertT(queueric,r,r->pathname);
                  } 
                  SIGNAL(&condrequest);
                  UNLOCK(&mtxrequest);
                  break;
               } 
            }
         }
      }
   }  
   return NULL;
}
// thread che elaboreranno le richieste
static void* worker()
{
   threadid=threadid+1;
   int thread=threadid;
   int cl=0;
   File* f;
   request* r;
   node* tofree;
   while(terminate)
   {
      LOCK(&mtxrequest);
      while( isEmpty(queueric) && terminate)
      {
         WAIT(&condrequest,&mtxrequest); 
      }
      if(terminate!=0)
      {
         tofree=takeHead(queueric);
         r=tofree->cont;
         UNLOCK(&mtxrequest);
         // prendo dalla testa della lista
         //scrivo sul log file
         LOG("Richiesta servita dal thread",0);
         LOG("TH ",thread);
         switch (r->OP)
         {
         case OPEN:      
            //CASO FLAG O_CREATE
            if(r->flags==O_CREATE)
            { 
               LOCK(&mtxhash);
               if(contain(Hash,r->pathname))
               {
                   UNLOCK(&mtxhash);
                  sendMessage(r->fd,YESCONTAIN);
               }
               else
               {
              
                  if(Hash->nelem+1<=sc.maxf)
                  {
                     UNLOCK(&mtxhash);
                     f=filecreate(r->pathname,r->fd);
                     LOCK(&(f->mtxf));
                        OpenFile(&f);
                     UNLOCK(&(f->mtxf))
                     LOCK(&mtxhash);
                        int k=hashins(&Hash,f->path_name,f); // inserisco il file
                     UNLOCK(&mtxhash);
                     if(k)sendMessage(r->fd,SUCCESS);
                     //SCRIVO NE LFILE DI LOG I FILE PRESENTI
                     LOG("Numero file presenti:",0);
                     LOG("NF ",Hash->nelem);          
                  
                  }
                  else
                  {
                     UNLOCK(&mtxhash);
                     f=filecreate(r->pathname,r->fd);
                     LOCK(&(f->mtxf));
                        OpenFile(&f);
                     UNLOCK(&(f->mtxf));
                     LOCK(&mtxhash);
                        hashins(&Hash,f->path_name,f); // inserisco il file
                     UNLOCK(&mtxhash);
                    
                     node* ftosend;
                     LOCK(&mtxfifo);
                        ftosend=takeHead(fifo);
                     UNLOCK(&mtxfifo);
                     sendMessage(r->fd,TOMUCHFILE);
                     File* fdel=(File*)ftosend->cont;

                     //dimensione
                     writen(r->fd,&fdel->dim,sizeof(int));
                     int len=strlen(fdel->path_name);
                     writen(r->fd,&len,sizeof(int));
                     //nomefile
                     writen(r->fd,fdel->path_name,len);
                     //contenuto
                     writen(r->fd,fdel->cont,fdel->dim);
                     free(ftosend->key);
                     free(ftosend);
                     LOCK(&mtxhash);
                       h_delete(&Hash,fdel->path_name); 
                     UNLOCK(&mtxhash);
                      //SCRIVO NE LFILE DI LOG I FILE PRESENTI
                     LOG("Numero file presenti:",0);
                     LOG("NF ",Hash->nelem); 
                  }
               }  
            }
            //caso flag O_LOCK
            if(r->flags==O_LOCK)
            {
               LOCK(&mtxhash);
               if(contain(Hash,r->pathname))
               {// apro il file in modaltà locked
                  File* f=getvalue(Hash,r->pathname);
                  UNLOCK(&mtxhash);
                  LOCK(&(f->mtxf));
                     LockFile(&f);
                     f->fd=r->fd;
                     OpenFile(&f);
                  UNLOCK(&(f->mtxf));
                  sendMessage(r->fd,SUCCESS);
                  LOG("LCK",0); 
               }
               else
               {
                  UNLOCK(&mtxhash);
                  sendMessage(r->fd,NOTPRESENT);
               }
            }
            //caso dei flag in or 
            if(r->flags==O_CRLK)
            {
               LOCK(&mtxhash);
               if(contain(Hash,r->pathname))
               {
                  UNLOCK(&mtxhash);
                  sendMessage(r->fd,YESCONTAIN);
               }
               else
               {
                  if(Hash->nelem+1<=sc.maxf)
                  {
                     UNLOCK(&mtxhash);
                     f=filecreate(r->pathname,r->fd);
                     LOCK(&(f->mtxf));
                        OpenFile(&f);
                        LockFile(&f);
                     UNLOCK(&(f->mtxf));
                     LOCK(&mtxhash);
                        int k=hashins(&Hash,f->path_name,f); 
                        if(k)sendMessage(r->fd,SUCCESS);
                     UNLOCK(&mtxhash);
                     //SCRIVO NE LFILE DI LOG I FILE PRESENTI
                     LOG("Numero file presenti:",0);
                     LOG("NF ",Hash->nelem);          
                  }
                  else
                  {
                     UNLOCK(&mtxhash);
                     f=filecreate(r->pathname,r->fd);
                     LOCK(&(f->mtxf));
                        OpenFile(&f);
                        LockFile(&f);
                     UNLOCK(&(f->mtxf));
                     LOCK(&mtxhash);
                        hashins(&Hash,f->path_name,f); // inserisco il file
                     UNLOCK(&mtxhash);
                     
                     node* ftosend;
                     LOCK(&mtxfifo);
                        ftosend=takeHead(fifo);
                     UNLOCK(&mtxfifo);

                     sendMessage(r->fd,TOMUCHFILE);
                     File* fdel=(File*)ftosend->cont;
                     //dimensione
                     writen(r->fd,&fdel->dim,sizeof(int));
                     //lunghezza pathname
                     int len=strlen(fdel->path_name);
                     writen(r->fd,&len,sizeof(int));
                     //nomefile
                     writen(r->fd,fdel->path_name,len);
                     //contenuto
                     writen(r->fd,fdel->cont,fdel->dim);
                     free(ftosend->key);
                     free(ftosend);
                     LOCK(&mtxhash);
                       h_delete(&Hash,fdel->path_name); // inserisco il file
                     UNLOCK(&mtxhash);
                     LOG("Numero file presenti:",0);
                     LOG("NF ",Hash->nelem); 
                  }
               }        
            }
            if(r->flags==O_OPEN) // non viene specificato alcun flag apre il file
            {
               LOCK(&mtxhash);
               if(contain(Hash,r->pathname))
               {
                  
                  File* f=getvalue(Hash,r->pathname);
                  UNLOCK(&mtxhash);
                  LOCK(&(f->mtxf));
                     OpenFile(&f);
                  UNLOCK(&(f->mtxf));
                  sendMessage(r->fd,SUCCESS);
               }
               else
               {
                  UNLOCK(&mtxhash);
                  sendMessage(r->fd,NOTPRESENT);
               }
            }
         break;

         case LOCKS:
               LOCK(&mtxhash);
               if(contain(Hash,r->pathname))
               {
                  
                  File* f=getvalue(Hash,r->pathname);
                  UNLOCK(&mtxhash);
                  LOCK(&(f->mtxf));
                  if(f->isopen==1)
                  {   
                     if(f->lock==1)
                     {  
                        if(f->fd==r->fd)
                        {
                           LockFile(&f);
                           UNLOCK(&(f->mtxf));
                           sendMessage(r->fd,SUCCESS);
                           LOG("LCK",0);
                        }
                        else
                        {
                           //se il file è già in stato di locked rinserisco la richiesta nella coda
                           UNLOCK(&(f->mtxf));
                           request *rnew=malloc(sizeof(request));
                           rnew->size=r->size;
                           rnew->flags=r->flags;
                           strcpy(rnew->pathname,r->pathname);
                           rnew->fd=r->fd;
                           rnew->OP=r->OP;
                           LOCK(&mtxrequest);
                              insertT(queueric,rnew,rnew->pathname);
                              SIGNAL(&condrequest);
                           UNLOCK(&mtxrequest);

                        }
                     }
                     else
                     {
                        LockFile(&f);
                        f->fd=r->fd;
                        UNLOCK(&(f->mtxf));
                        sendMessage(r->fd,SUCCESS);
                        LOG("LCK",0);
                        
                     }
                  }
                  else
                  {
                     UNLOCK(&(f->mtxf));
                     sendMessage(r->fd,NOTOPEN);
                  }
               }
               else
               { 
                  UNLOCK(&mtxhash); 
                  sendMessage(r->fd,NOTPRESENT);
               }
            break;

         case UNLOCKS:
               LOCK(&mtxhash);
               if(contain(Hash,r->pathname))
               {
                  File* f=getvalue(Hash,r->pathname);
                  UNLOCK(&mtxhash);
                  LOCK(&(f->mtxf));
                  if(f->fd==r->fd)
                  {
                     UnlockFile(&f);
                     UNLOCK(&(f->mtxf));
                     sendMessage(r->fd,SUCCESS);
                     LOG("ULK",0);
                  }
                  else
                  {
                     UNLOCK(&(f->mtxf));
                     sendMessage(r->fd,NOTPERMISSION);
                  }
               }
               else
               {
                  UNLOCK(&mtxhash);
                  sendMessage(r->fd,NOTPRESENT);
               }
            break;

         case WRITE:
               LOCK(&mtxhash);
               if(contain(Hash,r->pathname)==0)
               {
                  UNLOCK(&mtxhash);
                  sendMessage(r->fd,NOTPRESENT);
               }
               else
               {
                 
                  File *f=getvalue(Hash,r->pathname);
                  UNLOCK(&mtxhash);
                  LOCK(&(f->mtxf));
                  if(f->isopen==1 )
                  {
                     if(f->lock==1)
                     {
                        if(f->fd==r->fd)
                        {
                           appendFile(&f,r->contenuto,r->size);
                           UNLOCK(&(f->mtxf));
                           actual_size=actual_size+r->size;
                           //CONTROLLO DELLA DIMENSIONE
                           if(actual_size<sc.space)
                           {
                              sendMessage(r->fd,SUCCESS);
                              LOG("WR",f->dim);
                              
                           }
                           else
                           {
                              LOG("AR",0);
                              sendMessage(r->fd,SERVERFULL);
                             
                              LOCK(&mtxhash);
                                 Handlercapacity(r->fd);
                              UNLOCK(&mtxhash);
                           }
                           free(r->contenuto);
                           LOCK(&mtxfifo);
                              insertT(fifo,f,f->path_name); //inserisco nella coda fifo il nuovo elemento
                           UNLOCK(&mtxfifo);     
                           LOG("byte presenti:",0);
                           LOG("NB ",actual_size);                           
                        }
                        else
                        {
                           UNLOCK(&(f->mtxf));
                           sendMessage(r->fd,NOTPERMISSION);
                        }
                     }
                     else //caso con file senza locked non controllo  fd
                     {
                        appendFile(&f,r->contenuto,r->size);
                        UNLOCK(&(f->mtxf));  
                        actual_size=actual_size+r->size;                  
                        //CONTROLLO DELLA DIMENSIONE
                        if(actual_size<sc.space)
                        {
                           sendMessage(r->fd,SUCCESS);
                           LOG("WR",f->dim);
                        }
                        else
                        {
                           LOG("AR",0);
                           sendMessage(r->fd,SERVERFULL);
                           //inviare i file da espellere
                           LOCK(&mtxhash);
                              Handlercapacity(r->fd);
                           UNLOCK(&mtxhash);  
                        }
                        free(r->contenuto);
                        LOCK(&mtxfifo);
                           insertT(fifo,f,f->path_name); //inserisco nella coda fifo il nuovo elemento
                        UNLOCK(&mtxfifo);
                        LOG("byte presenti:",0);
                        LOG("NB ",actual_size);
                     }
                  }
                  else
                  {
                     UNLOCK(&(f->mtxf));
                     sendMessage(r->fd,NOTOPEN);
                  }
               }
            break;
         case APPEND:
               LOCK(&mtxhash);
               if(contain(Hash,r->pathname))
               {
                  
                  File* f=getvalue(Hash,r->pathname);
                  actual_size=actual_size+r->size;
                  UNLOCK(&mtxhash);

                  LOCK(&(f->mtxf));
                  if(f->isopen==1)
                  {
                     if(actual_size>sc.space)
                     {
                        //devo fare spazio
                        sendMessage(r->fd,SERVERFULL);
                        LOG("AR",0);
                        LOCK(&mtxhash);
                           Handlercapacity(r->fd);
                        UNLOCK(&mtxhash);
                     }
                     size_t news=f->dim+r->size;
                     char* newfile=malloc(sizeof(char)*news);
                     memcpy(newfile,f->cont,f->dim);
                     memcpy((newfile + f->dim),r->contenuto,r->size);
                     free(f->cont);
                     free(r->contenuto);
                     f->dim=f->dim+r->size;
                     f->cont=newfile;
                     UNLOCK(&(f->mtxf));
                     LOG("byte presenti:",0);
                     LOG("NB ",actual_size);
                     sendMessage(r->fd,SUCCESS);
                  }
                  else
                  {
                     UNLOCK(&(f->mtxf));
                     sendMessage(r->fd,NOTOPEN);
                  }
               }
               else
               {
                  UNLOCK(&mtxhash);
                  sendMessage(r->fd,NOTPRESENT);
               }
         break;

         case READ:
               LOCK(&mtxhash);
               if(contain(Hash,r->pathname))
               {
                  File* f=getvalue(Hash,r->pathname);
                  UNLOCK(&mtxhash);
                  LOCK(&(f->mtxf));
                  if(f->isopen==1)
                  {
                     if(f->lock==1) /*in caso sia in stato di lock controllo i permessi*/
                     {
                        if(f->fd==r->fd)
                        {
                           sendMessage(r->fd,SUCCESS);
                           sendFile(r->fd,f->cont,f->dim);
                           UNLOCK(&(f->mtxf));
                           LOG("RD",f->dim);
                        }
                        else
                        {
                           UNLOCK(&(f->mtxf));
                           sendMessage(r->fd,NOTPERMISSION);
                        }
                     }
                     else
                     {
                        sendMessage(r->fd,SUCCESS);
                        sendFile(r->fd,f->cont,f->dim); //invio il file
                        UNLOCK(&(f->mtxf));
                        LOG("RD",f->dim);
                     }         
                  }
                  else
                  {
                     UNLOCK(&(f->mtxf));
                     sendMessage(r->fd,NOTOPEN);
                  }
               }
               else
               {
                  UNLOCK(&mtxhash);
                  sendMessage(r->fd,NOTPRESENT);
                  //mando un messaggio di errore dato che non è presente
               }
            break;
         case READN:
            LOCK(&mtxhash);
            if(Hash->nelem==0)
            {
               UNLOCK(&mtxhash);
               sendMessage(r->fd,EMPTY);
            }
            else
            {
               sendMessage(r->fd,SUCCESS);
               readFiles(r->fd,r->flags);
               UNLOCK(&mtxhash);
            }
         break;

         case CLOSE:
               LOCK(&mtxhash);
               if(contain(Hash,r->pathname))
               {
                  
                  File* f=getvalue(Hash,r->pathname);
                  UNLOCK(&mtxhash);
                  LOCK(&(f->mtxf));
                     CloseFile(&f);
                  UNLOCK(&(f->mtxf));
                  sendMessage(r->fd,SUCCESS);
                  LOG("CL",0);
               }
               else
               {
                  UNLOCK(&mtxhash);
                  sendMessage(r->fd,NOTPRESENT);
               }
         break;

         
         case REMOVE:
               LOCK(&mtxhash);
               if(contain(Hash,r->pathname))
               {
                  File* f=getvalue(Hash,r->pathname);
                  UNLOCK(&mtxhash);
                  LOCK(&(f->mtxf));
                  if(f->lock==0)
                  {
                     UNLOCK(&(f->mtxf));
                     sendMessage(r->fd,NOTLOCKED);
                  }
                  else
                  {
                     if(f->fd==r->fd)
                     {
                        UNLOCK(&(f->mtxf));
                        LOCK(&mtxhash);
                           actual_size=actual_size-f->dim;
                           h_delete(&Hash,f->path_name); // cancello il file
                        UNLOCK(&mtxhash);
                        LOCK(&mtxfifo);
                           delete(fifo,f->path_name); // elimino il file anche dalla coda FIFO
                        UNLOCK(&mtxfifo);
                        free(f->cont);
                        free(f);
                        
                        sendMessage(r->fd,SUCCESS);
                     }
                     else
                     {
                        // il client non ha i permessi
                        UNLOCK(&(f->mtxf));
                        sendMessage(r->fd,NOTPERMISSION);
                     }
                  }
                  
               }
               else
               {
                  // il file non è presente
                  UNLOCK(&mtxhash);
                  sendMessage(r->fd,NOTPRESENT);
               }
            break;
         case CLOSECONNECTION:
               cl=1;
               LOCK(&mtxcon);
               FD_CLR(r->fd,&set);
               activecon=activecon-1;
               if(activecon==0 && closesighup==0)
               {
                  SIGNAL(&condcon);
               }
               sendMessage(r->fd,SUCCESS);
               relaselock(r->fd);
               close(r->fd);
               UNLOCK(&mtxcon);
         break;
            
         }
         int tmp=r->fd;
         if(cl==0) //se la connessione non è stata chiusa
         {
            writen(pipefd[1],&tmp,sizeof(int));
         }
         
         cl=0;
         free(r);
         free(tofree->key);
         free(tofree);
      }
      else
      {
         UNLOCK(&mtxrequest); //lascio la lock
      }
   }
   return NULL;
}
// ottiene la configurazione dal file config.txt
void configura(char * nomefile)
{
   int n=300;
   FILE* fdc;
   char* line1;
   char* valore;
   char* campo;
   char temp[n];
   char* buff=malloc(sizeof(char)*n);
   if((fdc=fopen(nomefile,"r"))==NULL)
   {
      perror("config.txt, in apertura");
      
   }
   while(fgets(buff,n,fdc)!=NULL)
   {
      strncpy(temp,buff,strlen(buff)+1);
      campo= strtok_r(temp,":", &line1);
      valore=strtok_r(NULL,":",&line1);
      if(strcmp(campo,"THREADS")==0)
      {
        sc.nwork=strtol(valore,NULL,10);
      }
      if(strcmp(campo,"SIZE")==0)
      {
         sc.space=strtol(valore,NULL,10);
      }
      if(strcmp(campo,"NFILE")==0)
      {
         sc.maxf=strtol(valore,NULL,10);
      }
      if(strcmp(campo,"NAME")==0)
      {
         sc.socketname=malloc(sizeof(char)*strlen(valore)+1);
         strncpy(sc.socketname, valore,strlen(valore)+1);
         sc.socketname[strlen(sc.socketname)-1]='\0';
      }
      if(strcmp(campo,"LOGFILE")==0)
      {
         sc.filelog=malloc(sizeof(char)*strlen(valore)+1);
         strncpy(sc.filelog,valore,strlen(valore)+1);
         if((logfd=fopen(sc.filelog,"w" ))==NULL)//apro il file di log
         {
            perror("s.c file log, in apertura");
            exit(EXIT_FAILURE);
         }   
      }
   }
   fclose(fdc);
   free(buff);
}

//funzione per leggere le richieste del client
request* readMessage(int client)
{
   request *r=malloc(sizeof(request));
   void* cont;
   int k;
   if((k=readn(client,r,sizeof(request)))==-1)
   {
      perror("ERRORE:read dimensione");    
   }
   if(k==0)
   {
      free(r);
      r=NULL;
   }
   else
   {
      if(r->size!=0) //caso in cui il client scrive anche il contenuto del file
      {
         cont=malloc(r->size);
         if((k=readn(client,cont,r->size))==-1)
         {
            perror("ERRORE:read request");      
         }
         if(k==0)
         {
            r=NULL;
         }
         else
         {
            r->contenuto=cont;
            r->fd=client;
         }
      }
      else
      {
         r->contenuto=NULL;
      }
      if(r!=NULL)
      {
          r->fd=client;
      }
   }
   return r;
}

void* signalHandler()
{
   sigset_t sets;
   int signal;
   int fakewrite=-1;
   sigemptyset(&sets);
   sigaddset(&sets, SIGTERM);
   sigaddset(&sets, SIGINT);
   sigaddset(&sets, SIGQUIT);
   sigaddset(&sets, SIGHUP);  
   ///SEGNALI CHE DEVO INTERCETTARE
   pthread_sigmask(SIG_SETMASK, &sets, NULL);

   if (sigwait(&sets, &signal) != 0) 
   {
      return NULL;
   }
   if (signal == SIGINT || signal == SIGQUIT) { 
      
      writen(pipedisp[1],&fakewrite,sizeof(int)); 
      terminate=0;
      pthread_join(disp,NULL); //attendo il dispatcher
      LOCK(&mtxrequest);
         BCAST(&condrequest);
      UNLOCK(&mtxrequest);
      //attendo i worker
      for(int i=0;i<sc.nwork;i++)
      {
         pthread_join(threadp[i],NULL);
      }
   } else if (signal == SIGHUP) { 
      
      closesighup=0; //non accetto più connessioni
      LOCK(&mtxcon);
      //aspetto che non ci siano più connessioni attive
      while(activecon!=0)
      {
         WAIT(&condcon,&mtxcon); 
      }
      UNLOCK(&mtxcon);
      terminate=0;
      writen(pipedisp[1],&signal,sizeof(int));
      //devo attendere la close connection
      pthread_join(disp,NULL); //attendo il dispatcher
      LOCK(&mtxrequest);
         BCAST(&condrequest);
      UNLOCK(&mtxrequest);
      //atendo i worker
      for(int i=0;i<sc.nwork;i++)
      {
         pthread_join(threadp[i],NULL); 
      }
   }
   //scrivo sul log file i file rimasti nel server
   for(int i=0;i<sc.maxf;i++ )
   {
      Lis curr=Hash->buckets[i];
      if(curr!=NULL)
      {
         if(curr->length>0)
         {
            node* cur=curr->header;
            while(cur!=NULL)
            {
               LOG_file(cur->key);
               cur=cur->next;
            }
         }
      }
   }
   ///svuoto le strutture del server
   freeStruct();
   //svuoto in caso la coda delle richieste
   free(sc.socketname);
   free(sc.filelog);
   free(threadp);
   close(fd_skt);
   printf("-------------------STATISTICHE-------------------\n\n");
   system("./script/scriptstat.sh");
   printf("-------------------------------------------------\n\n");
   return NULL;
}

//invia l'esito delle operazioni
void sendMessage(int client, int message)
{
   int nbyte=0;
   nbyte=writen(client,&message,sizeof(int));
   if(nbyte==-1)
   {
      perror("s.c:write");
   }
}

//invia il contenuto di un file
void sendFile(int client,char* cont,int dim)
{
   int nbyte=0; 
   //scrivo la dimensione del file poi il contenuto
   writen(client,&dim,sizeof(int));
   nbyte=writen(client,cont,dim);
   
   if(nbyte==-1)
   {
      perror("s.c:write");
   }
}

//gestione dei capacity miss
void Handlercapacity(int fd)
{
   Lis ftosend=create();// lsita dei file da spedire
   int number=0; //numero di file che il server invierà al client
   File* f;
   node* ft;
   while(actual_size>sc.space)
   {
      LOCK(&mtxfifo);
         ft= takeHead(fifo);
      UNLOCK(&mtxfifo);
      f=ft->cont;
      actual_size=actual_size-f->dim;
      insertT(ftosend,ft->cont,ft->key); // inserisco nella coda dei file da spedire
      free(ft->key);
      free(ft);      
      number++;
   }
   //invio numero di file
   writen(fd,&number,sizeof(int));
   for(int i=0;i<number;i++)
   {
      node* temp=takeHead(ftosend);
      f=temp->cont;
      //dimensione
      writen(fd,&f->dim,sizeof(int));
      //lunghezza pathname
      int len=strlen(f->path_name);
      writen(fd,&len,sizeof(int));
      //nomefile
      writen(fd,f->path_name,len);
      //contenuto
      writen(fd,f->cont,f->dim);
      LOG("RD",f->dim);
      h_delete(&Hash,f->path_name);
      free(f->cont);
      free(f);
      free(temp->key);
      free(temp);
   }
   // svuoto la lista appena creata
   free(ftosend); 
}

//restituisce al client n file (funzione ReadNFiles)
void readFiles(int fd,int n)
{
   int i=0;
   int number=0;
   if(n<=0)
   {
      number=Hash->nelem;
   }
   else
   {
      if(n>Hash->nelem)
      {
         number=Hash->nelem;
      }
      else
      {
         number=n;
      }
   }
   writen(fd,&number,sizeof(int));
   while(number>0 && i<Hash->size )
   {
      Lis curr=Hash->buckets[i];
      if(curr!=NULL)
      {
         if(curr->length>0)
         {
            node* cur=curr->header;
            while(cur!=NULL)
            {
               // devo inviare il file
               File* f=cur->cont;
               //dimensione
               writen(fd,&f->dim,sizeof(int));
               //lunghezza pathname
               int len=strlen(f->path_name)+1;
               writen(fd,&len,sizeof(int));
               //nomefile
               writen(fd,f->path_name,len);
               //contenuto
               writen(fd,f->cont,f->dim);
               LOG("RD",f->dim);
               number--;
               cur=cur->next;
            }
         }
      }
      i++;
   } 
}

//scrive le informazioni sul file di log  
void LOG(char* log, int info)
{
   char*buf=malloc(sizeof(char)*strlen(log)+1);
   strncpy(buf,log,strlen(log));
   LOCK(&mtxlog);
   if(logfd!=NULL)
   {
      if(info!=0)
      {
         fprintf(logfd,"%s: %d\n" ,log,info); 
      }
      else
      {
         fprintf(logfd,"%s\n",log);
      }
      fflush(logfd);
   }
   UNLOCK(&mtxlog);
   free(buf);
}

//funzione che scrive i file rimasti nel server nel file di log
void LOG_file(char* log)
{
   char*buf=malloc(sizeof(char)*strlen(log)+1);
   strncpy(buf,log,strlen(log));
   LOCK(&mtxlog);
   if(logfd!=NULL)
   {
      fprintf(logfd,"LF %s\n",log);
      fflush(logfd);
   }
   UNLOCK(&mtxlog);
   free(buf);
}

//Libera le struct, funzione richiamata alla terminazione del server
void freeStruct()
{
   for(int i=0; i<Hash->size ; i++ )
   {
      Lis curr=Hash->buckets[i];
      if(curr!=NULL)
      {
         if(curr->length>0)
         {
            node* cur=curr->header;
            while(cur!=NULL)
            {
               node * tmp=cur;
               cur=cur->next;
               File* f=tmp->cont;
               free(tmp->key);
               free(f->cont);
               free(tmp->cont);
               free(tmp);
            }
         }
      }
      free(curr);
   }
   free(Hash->buckets);
   free(Hash);
   node *cur=fifo->header;
   while(cur!=NULL)
   {
      node * tmp=cur;
      cur=cur->next;     
      free(tmp->key);
      free(tmp);
   }
   free(fifo);
   free(queueric);
   fclose(logfd);
}

//rilascia le lock sui file di quel fd
void relaselock(int fd)
{
   LOCK(&mtxhash);
   for(int i=0; i<Hash->size ; i++ )
   {
      Lis curr=Hash->buckets[i];
      if(curr!=NULL)
      {
         if(curr->length>0)
         {
            node* cur=curr->header;
            while(cur!=NULL)
            {
               node * tmp=cur;
               cur=cur->next;
               File* f=tmp->cont;
               LOCK(&(f->mtxf));
               if(f->fd==fd && f->lock==1)
               {
                  UnlockFile(&f);
                  LOG("ULK",0);
               }
               UNLOCK(&(f->mtxf));
            }
         }
      }
   }
   UNLOCK(&mtxhash);
}
