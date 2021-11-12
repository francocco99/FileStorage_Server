
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


#define UNIX_PATH_MAX 108
#define N 300
#define SOCKNAME "/mysock"
static void* worker();
static void* dispatcher();
void LOG_file(char* log);
void configura(char* nomefile);
request * readMessage(int client);   /// metodo per la lettura dei messaggi
void sendMessage(int client, int message); // metodo per la scrittura di messagi sulla socket
void sendFile(int client,char* cont,int dim); // per mandare i messaggi
static inline int readn(long fd, void *buf, size_t size); // lettura n byte
static inline int writen(long fd, void *buf, size_t size); // scrittura n byte
void readFiles(int fd,int n);// legge n file dal server
void Handlercapacity(int fd);
void LOG(char* log, int info);
void freeStruct();
static void* signalHandler();
typedef struct serverconf
{
      int nwork; // numero worker da predere dal file di config
      int space; // spazio massimo
      int maxf; // numero massimo di file
      char* socketname; // nome della socket
      char* filelog;
}ServerConf;




pthread_mutex_t mtxrequest = PTHREAD_MUTEX_INITIALIZER; 
pthread_cond_t condrequest = PTHREAD_COND_INITIALIZER; 

pthread_mutex_t mtxhash = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condhash = PTHREAD_COND_INITIALIZER; 

pthread_mutex_t mtxlog = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condlog=PTHREAD_COND_INITIALIZER;


pthread_mutex_t mtxfifo= PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condfifo= PTHREAD_COND_INITIALIZER;
int pipefd[2];
ServerConf sc;
//Queue* queueric;
 // lista di fd del client
hash* Hash;
// terminazione worker con segnali
int terminate=1; 
int gestore=1;
int closesighup=1;

int threadid=0;

int fd; //indice per utilizzare 
int fdc, 
   fd_skt; // scoket di connessione
int fd_c;  // che serve per scorrere
int fd_num=0; //numero connessioni
int activecon=0;
fd_set set, // tutti i file descriptor attivi
   rdset;   // insieme di file pronti per la lettura

FILE* logfd=NULL; // filedescriptor log file
Lis fifo;   //coda per la gestione fifo
Lis queueric;
int actual_size=0;
pthread_t disp;
pthread_t Hand;
pthread_t* threadp;
int main(int argc,char* argv[])
{
   
  // queueric=Queue_create();
   configura(argv[1]);
   Hash=h_create(sc.space); //creazione tabella hash
   fifo=create();
   queueric=create();
      ////SETUP SERVER
   unlink(SOCKNAME);
   //creo la pipe
   pipe(pipefd);
   
   struct sockaddr_un sa;
   strncpy(sa.sun_path,SOCKNAME,UNIX_PATH_MAX);
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
   // attivazione thread dispatcher
   
   pthread_create(&disp,NULL,&dispatcher,NULL);
   pthread_create(&Hand,NULL,&signalHandler,NULL);
   pthread_join(Hand,NULL);

   
}


// si occupa di attivare i thread worker che gestiranno le richieste cioè i worker
static void* dispatcher()
{
   listen(fd_skt,SOMAXCONN);

   /////////// da capire
   FD_ZERO(&set);
   FD_SET(fd_skt,&set);
   if (fd_skt > fd_num) fd_num = fd_skt;
   FD_SET(pipefd[0],&set); // fd_ske è il descrittore su  cui vado a fare l'accept
   if (pipefd[0] > fd_num) fd_num = pipefd[0]; // mantengo il massimo indice di descrittoreattivo in fd_num 
  
   
 ////////////////
   while(gestore)
   {
      rdset=set; // inizializzo ogni volta dato che la select modifica wrset
      struct timeval timeout;
                timeout.tv_sec = 0;
                timeout.tv_usec = 1;
      if(select(fd_num+1,&rdset,NULL,NULL,&timeout)==-1)
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
                  fd_c=accept(fd_skt,NULL,0); // questa accept non si blocca 
                  activecon++;
                  printf("HO ACCETTATO UNA RICHIESTA: %d\n",fd_c);
                  LOG("Numero di connessioni attive ",0);
                  LOG("NC: ",activecon);
                  FD_SET(fd_c,&set);
                  if (fd_c > fd_num){fd_num = fd_c;}
                  break;
               }
              
               else if(fd==pipefd[0])
               {
                 
                  int fd_o;
                  readn(pipefd[0],&fd_o,sizeof(int));
                  FD_SET(fd_o,&set);
                  break;
               }
               else // PER LA lettura 
               {
                  
                  printf("lettura:%d\n",fd);
                             // aggiorno la lista
                  FD_CLR(fd,&set);

                  request *r;
                  r=readMessage(fd);            
                  LOCK(&mtxrequest);
                  if(r!=NULL)
                  {
                     //Queue_enqueue(queueric,r);
                     insertT(queueric,r,r->pathname);
                  }        
                     ////FARE REALLOC FORSE
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
   int close=0;
   File* f;
   request* r;
   node* tofree;
   while(terminate)
   {
      LOCK(&mtxrequest);
      while(/*Queue_isempty(queueric)*/ isEmpty(queueric) && terminate)
      {
         WAIT(&condrequest,&mtxrequest); 
      }
      printf("%d\n",terminate);
      if(terminate!=0)
      {
        /* request* r=Queue_dequeue(queueric);*/
         tofree=takeHead(queueric);
         r=tofree->cont;
         UNLOCK(&mtxrequest);
         // prendo dalla testa della lista
         
         printf("NOMEFILE:%s \n",r->pathname);
         printf("OPERAZIONE:%d\n",r->OP);
         printf("FLAGS:%d\n\n\n",r->flags);
         //scrivo sul log file
         LOG("Richiesta servita dal thread",0);
         LOG("TH ",thread);
         switch (r->OP)
         {
         case OPEN:      
            //CASO FLAG O_CREATE
            if(r->flags==O_CREATE)
            { 
               if(contain(Hash,r->pathname))
               {
                  printf("Error:si contiene\n");
                  sendMessage(r->fd,YESCONTAIN);
               }
               else
               {
                  if(Hash->nelem<=sc.maxf)
                  {
                     f=filecreate(r->pathname,r->fd);
                     OpenFile(&f);
                     LOCK(&mtxhash);
                        int k=hashins(&Hash,f->path_name,f); // inserisco il file
                        if(k)sendMessage(r->fd,SUCCESS);
                     UNLOCK(&mtxhash);
                     //SCRIVO NE LFILE DI LOG I FILE PRESENTI
                     LOG("Numero file presenti:",0);
                     LOG("NF ",Hash->nelem);
                     printf("nelementi%d\n",Hash->nelem);
                     
                  }
                  else
                  {
                     node* ftosend;
                     LOCK(&mtxhash);
                        ftosend=takeHead(fifo);
                        h_delete(&Hash,r->pathname);
                     UNLOCK(&mtxhash);
                     sendMessage(r->fd,TOMUCHFILE);
                     File* f=(File*)ftosend->cont;
                     sendFile(f->fd,f->cont,f->dim);
                     free(ftosend);
                     //non posso più inserire file  
                     //gestione di rinvio del file in testa alla coda fifo
                  }
               }  

            }
            //caso flag O_LOCK
            if(r->flags==O_LOCK)
            {
               if(contain(Hash,r->pathname))
               {// apro il file in modaltà lockedF
                  LOCK(&mtxhash);
                     File* f=getvalue(Hash,r->pathname);
                     LockFile(&f);
                  UNLOCK(&mtxhash);
                  sendMessage(r->fd,SUCCESS);
               }
               else
               {
                  sendMessage(r->fd,NOTPRESENT);
               }
            }
            //caso dei flag in or manca
            if(r->flags==0) // non viene specificato alcun flag
            {
               if(contain(Hash,r->pathname))
               {
                  LOCK(&mtxhash);
                  File* f=getvalue(Hash,r->pathname);
                  f->isopen=1;
                  UNLOCK(&mtxhash);
                  sendMessage(r->fd,SUCCESS);
               }
               else
               {
                  sendMessage(r->fd,NOTPRESENT);
               }
            }
         break;

         case LOCKS:
                  if(contain(Hash,r->pathname))
                  {
                     LOCK(&mtxhash);
                     File* f=getvalue(Hash,r->pathname);
                     if(f->fd==r->fd)
                     {
                        LockFile(&f);
                        sendMessage(r->fd,SUCCESS);
                        LOG("LCK",0);
                     }
                     else
                     {
                        sendMessage(r->fd,NOTPERMISSION);
                     }
                     UNLOCK(&mtxhash);
               }
               else
               {
                  
                  sendMessage(r->fd,NOTPRESENT);
               }
            break;

         case UNLOCKS:
               if(contain(Hash,r->pathname))
               {
                  LOCK(&mtxhash);
                     File* f=getvalue(Hash,r->pathname);
                     if(f->fd==r->fd)
                     {
                        f->lock=0;
                        sendMessage(r->fd,SUCCESS);
                        LOG("ULK",0);
                     }
                     else
                     {
                        sendMessage(r->fd,NOTPERMISSION);
                     }

                  UNLOCK(&mtxhash);
               }
               else
               {
                  sendMessage(r->fd,NOTPRESENT);
               }
            break;
         case WRITE:
               if(contain(Hash,r->pathname)==0)
               {
                  sendMessage(r->fd,NOTPRESENT);
               }
               else
               {
                  
                     LOCK(&mtxhash);
                     File *f=getvalue(Hash,r->pathname);
                     if(f->isopen==1 )
                     {
                        if(f->fd==r->fd)
                        {
                           f->dim=r->size;
                           actual_size=actual_size+r->size;
                           LOG("byte presenti:",0);
                           LOG("NB ",actual_size);
                           
                           f->cont=malloc(f->dim);
                           memcpy(f->cont,r->contenuto,r->size);
                     UNLOCK(&mtxhash);  
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
                              Handlercapacity(r->fd);
                           }
                           free(r->contenuto);
                           LOCK(&mtxfifo);
                           insertT(fifo,f,f->path_name); //inserisco nella coda fifo il nuovo elemento
                           UNLOCK(&mtxfifo);                              //aumentano il numero degli elementi presenti nel server
                           printf("TESTA CODA FIFO\n");
                           printf("%s\n",fifo->header->key);
                           
                           
                        }
                        else
                        {
                           sendMessage(r->fd,NOTPERMISSION);
                        }
                     
                     
                     }
                     else
                     {
                        sendMessage(r->fd,NOTOPEN);
                     }
               }
            break;
         case APPEND:
               
               if(contain(Hash,r->pathname))
               {
                  File* f=getvalue(Hash,r->pathname);
                  if(f->isopen==1)
                  {
                     //leggi il file in append
                     f->dim=f->dim+r->size;
                     if(f->dim>sc.space)
                     {
                        //devo fare spazio
                        sendMessage(r->fd,SERVERFULL);
                        Handlercapacity(r->fd);
                     }
                     void* newfile=malloc(f->dim+r->size);
                     memcpy(newfile,f->cont,f->dim);
                     memcpy(newfile+f->dim,r->contenuto,r->size);
                     free(f->cont);
                     free(r->contenuto);
                     f->cont=newfile;
                     sendMessage(r->fd,SUCCESS);
                     
                  }
                  else
                  {
                     sendMessage(r->fd,NOTOPEN);
                  }
               }
               else
               {
                  sendMessage(r->fd,NOTPRESENT);
               }


            break;

         case READ:
               if(contain(Hash,r->pathname))
               {
                  File* f=getvalue(Hash,r->pathname);
                  if(f->isopen==1)
                  {
                     //invio il file
                     sendMessage(r->fd,SUCCESS);
                     sendFile(r->fd,f->cont,f->dim);
                     LOG("RD",f->dim);
                  }
                  else
                  {
                     sendMessage(r->fd,NOTOPEN);
                  }
               }
               else
               {
                  sendMessage(r->fd,NOTPRESENT);
                  //mando un messaggio di errore dato che non è presente
               }
            break;
         case READN:
            if(Hash->nelem==0)
            {
               sendMessage(r->fd,EMPTY);
            }
            else
            {
               
               sendMessage(r->fd,SUCCESS);
               readFiles(r->fd,r->flags);
            }
         break;
         case CLOSE:
               if(contain(Hash,r->pathname))
               {
                  LOCK(&mtxhash);
                     File* f=getvalue(Hash,r->pathname);
                     f->isopen=0;
                  UNLOCK(&mtxhash);
                  sendMessage(r->fd,SUCCESS);
                  LOG("CL",0);
               }
               else
               {
                  sendMessage(r->fd,NOTPRESENT);
               }

            break;

         
         case REMOVE:
               if(contain(Hash,r->pathname))
               {
                  File* f=getvalue(Hash,r->pathname);
                  if(f->lock==0)
                  {
                     sendMessage(r->fd,NOTLOCKED);
                  }
                  else
                  {
                     if(f->fd==r->fd)
                     {
                        LOCK(&mtxhash);
                           actual_size=actual_size-f->dim;
                           h_delete(&Hash,r->pathname); // cancello il file
                           delete(fifo,f->path_name); // elimino il file anche dalla coda FIFO

                        UNLOCK(&mtxhash);
                        sendMessage(r->fd,SUCCESS);
                     
                        //si può rimuovere il file
                     }
                     else
                     {
                        // il client non ha i permessi
                        sendMessage(r->fd,NOTPERMISSION);
                     }
                  }
                  
               }
               else
               {
                  // il file non è presente
                  sendMessage(r->fd,NOTPRESENT);
               }
            break;
         case CLOSECONNECTION:

               FD_CLR(r->fd,&set);
               close=1;
               //decidere cosa fare

         break;
            
         }
         int tmp=r->fd;
         if(close==0)
         {
            writen(pipefd[1],&tmp,sizeof(int));
         }
         else
         {
           terminate=0;
         }
         free(r);
         free(tofree->key);
         free(tofree);
      }
      else
      {
           UNLOCK(&mtxrequest); //lascio la lock
      }
   }
   printf("IO HO TERMINATO\n");
   return NULL;
}
// ottiene la configurazione del file REMINDER: sarebbe opportuno passarlo con argv[0] il file config
void configura(char * nomefile)
{
   printf("%s\n",nomefile);
   FILE* fdc;
   char* line1;
   char* valore;
   char* campo;
   char temp[N];
   char* buff=malloc(sizeof(char)*N);
   //char* line;
   if((fdc=fopen(nomefile,"r"))==NULL)
   {
      perror("config.txt, in apertura");
      exit(EXIT_FAILURE);
   }
   while(fgets(buff,N,fdc)!=NULL)
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
        strcpy(sc.socketname, valore);
      }
      if(strcmp(campo,"LOGFILE")==0)
      {
         sc.filelog=malloc(sizeof(char)*strlen(valore)+1);
         strcpy(sc.filelog, valore);
         printf("%s",sc.filelog);
         sc.filelog[strlen(sc.filelog)-1]='/0';
         if((logfd=fopen(sc.filelog,"w" ))==NULL)//apro il file di log
         {
            perror("s.c file log, in apertura");
               exit(EXIT_FAILURE);
         } 
         
               
      }
     
   }
    fclose(fdc);
   free(buff);
   printf("%d\n",sc.nwork);
   printf("%d\n",sc.space);
   printf("%d\n",sc.maxf);
   printf("%s\n",sc.socketname);

}

request* readMessage(int client)
{
   request *r=malloc(sizeof(request));
   void* cont;
   int k;
   if((k=readn(client,r,sizeof(request)))==-1)
   {
      perror("ERRORE:read dimensione");  
      exit(EXIT_FAILURE);    
   }
   if(k==0)
   {
      free(r);
      printf("Ho raggiunto l' EOF\n");
      r=NULL;
   }
   else
   {
      if(r->size!=0)
      {
         printf("SIZE DEL CONTENUTO %d\n",r->size);
         cont=malloc(r->size);
         if((k=readn(client,cont,r->size))==-1)
         {
            perror("ERRORE:read request");      
         }
         if(k==0)
         {
            printf("Ho raggiunto l' EOF\n");
            r=NULL;
            //da modificare
         // exit(EXIT_FAILURE);
         
         }
         else
         {
           // printf("CONTENUTO:%s",(char*)cont);
           /*r->contenuto=malloc(r->size);
           memcpy(r->contenuto,cont,r->size);
           free(cont);*/
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
   sigemptyset(&sets);
   sigaddset(&sets, SIGTERM);
   sigaddset(&sets, SIGINT);
   sigaddset(&sets, SIGQUIT);
   sigaddset(&sets, SIGHUP);  
   ///SEGNALI CHE DEVO INTERCETTARE
   pthread_sigmask(SIG_SETMASK, &sets, NULL);

   if (sigwait(&sets, &signal) != 0) 
   {

   }
   if (signal == SIGINT || signal == SIGQUIT) {  
      
      terminate=0;
      gestore=0;
     /* fd_num=-1;
       FD_ZERO(&rdset);
       FD_ZERO(&set);*/
      pthread_join(disp,NULL); //attendo il dispatcher
      LOCK(&mtxrequest);
         BCAST(&condrequest);
      UNLOCK(&mtxrequest);

      for(int i=0;i<sc.nwork;i++)
      {
         pthread_join(threadp[i],NULL);
        
      }
   } else if (signal == SIGHUP) { 
      //FAI QUALCOS'altro
      
   }
   for(int i=0;i<sc.space;i++ )
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
            printf("\n");
         }
      }
   }
   ///FARE LE FREE
   freeStruct();
   //svuoto in caso la coda delle richieste
   free(sc.socketname);
   free(sc.filelog);
   free(threadp);
   system("./script/scriptstat.sh");
   return NULL;
}
void sendMessage(int client, int message)
{
   //writen qua 
   int nbyte=0;
   nbyte=writen(client,&message,sizeof(int));
  // printf("WRITE: bytescritti: %d\n",nbyte);
   if(nbyte==-1)
   {
      perror("s.c:write");
      exit(EXIT_FAILURE);
   }


}
void sendFile(int client,char* cont,int dim)
{
   int nbyte=0; 
   //writen qua
   writen(client,&dim,sizeof(int));
   nbyte=writen(client,cont,dim);
   
   if(nbyte==-1)
   {
      perror("s.c:write");
      exit(EXIT_FAILURE);
   }

}
void Handlercapacity(int fd)
{
   Lis ftosend=create();
   int number=0; //numero di file che il server invierà al client
   File* f;
   node* ft;
   printf("actualsize %d\n",actual_size);
   printf("space server %d\n",sc.space);
   while(actual_size>sc.space)
   {
      LOCK(&mtxfifo);
         ft= takeHead(fifo);
      UNLOCK(&mtxfifo);
      f=ft->cont;
       LOCK(&mtxhash);
         actual_size=actual_size-f->dim;
       UNLOCK(&mtxhash);
      insertT(ftosend,ft->cont,ft->key); // inserisco nella coda dei file da spedire
      free(ft->key);
      free(ft);      
      number++;
   }
      
   printf("NUMERO DI FILE DA INVIARE: %d\n",number);
   //invio numero di file
   writen(fd,&number,sizeof(int));
   for(int i=0;i<number;i++)
   {
      node* temp=takeHead(ftosend);
      f=temp->cont;
      //dimensione
      writen(fd,&f->dim,sizeof(int));
      ///forse scrivere la dimenisone del nome file
      int len=strlen(f->path_name);
      writen(fd,&len,sizeof(int));
      //nomefile
      writen(fd,f->path_name,len);
      //contenuto
      writen(fd,f->cont,f->dim);
      LOG("RD",f->dim);
      LOCK(&mtxhash);
         h_delete(&Hash,f->path_name);
       UNLOCK(&mtxhash);
      free(temp->key);
      free(temp);
   }
   // svuoto la lista appena creata
   free(ftosend);
   
}
void readFiles(int fd,int n)
{
   int i=0;
   if(n<=0)
   {
      printf("Leggo tutti i File\n");
   }
   else
   {
      printf("Leggo solo alcuni File\n");
      if(n>Hash->nelem)
      {

         writen(fd,&Hash->nelem,sizeof(int));
      }
      else
      {
         printf("number che invio %d\n",n);
         writen(fd,&n,sizeof(int));
         while(n>0 && i<Hash->size )
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
                     ///forse scrivere la dimenisone del nome file
                     int len=strlen(f->path_name)+1;
                     writen(fd,&len,sizeof(int));
                     //nomefile
                     writen(fd,f->path_name,len);
                     //contenuto
                     writen(fd,f->cont,f->dim);
                     LOG("RD",f->dim);
                     n--;
                     cur=cur->next;
                  }
                  printf("\n");
               }
            }
            i++;
         }
      } 
   }
}



//////////////////////////// FUNZIONI PER LETTURA E SCRITTURA DA SOCKET /////////////////////////////////////////

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
//////////////////////////// FUNZIONI PER LETTURA E SCRITTURA DA SOCKET /////////////////////////////////////////
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
   /*free(queueric->queue);
   free(queueric);*/
   free(queueric);
   fclose(logfd);
}
