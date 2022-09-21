#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include "../header/api.h"
#include "../header/stringutil.h"
#include "../header/util.h"
#include "../header/Protcol.h"


#define UNIX_PATH_MAX 108
// Stampa il comando help
void PrintCommand(); 
// funzione per creare un file e scriverlo 
int  CreateWrite(char** ric,int dim);
//funzione utilizzata da visitaDir
void visitaDir(char* nomedir);
//procedura chiamta da -r
int letturaFiles(char** ric, int dim);
//procedura chamta da -R
void letturaRandom(int n);
//funzione utilizzata da visitaDir
int isdot(const char dir[]);
//procedura richiamta da visitaDir richiede la scrittura del file sul server
void writeF(char* pathname);
//procedura per il controllo dei parametri
void controlFirstStr(char** argv, int argc);
//funzione chiamata dall'opzione -a
int append(char* namefile,char* contentfile);

char* backupFolder;
char* downloadFolder;
int result=0;
//Flag per il controllo per gli argomenti
int f,p,h,D,d,t,w,W,r,R=0;

//seconds utilizzata per l'opzione -t
//nvisit per la funzione VistaDir
int seconds,nvisit=0;

int main(int argc, char* argv[])
{
  
   int sec= 1000; 
   struct timespec abs;
   clock_gettime(CLOCK_REALTIME,&abs); 
   abs.tv_sec=abs.tv_sec+10;
   //prendo l'orario corrente  e ci sommo 10 secondi
   int opt;  
   char* namesock;
   controlFirstStr(argv,argc);

   if(p==1)
      printf("---------------------CLIENT----------------------\n");

   while ((opt = getopt(argc,argv, ":a:h:f:w:W:D:r:R:d:t:l:u:c:p")) != -1) 
   {  
      int dim;
      char** ric;
      char* temp1;
      switch(opt) 
      {
         case 'f':   
            namesock=malloc(sizeof(char)*strlen(optarg)+1);
            strcpy(namesock,optarg);   
            if(openConnection(optarg,sec,abs)==-1)
            {
               if(p==1)
               {
                  printf("Errore: impossibile connettersi\n");
                  exit(EXIT_FAILURE);
               }
            }
            else
            {
               if(p==1)
               {
                  printf("Client connesso correttamente\n\n");
               }
            }
         break;

         case 'p': 
            abilitastampe();          
         break;

         case 'w':     
            dim=parse_str(&ric,optarg,",");
            if(dim==2)
            {
               char* temp1;
               strtok_r(ric[1],"=", &temp1);   
               int n=strtol(strtok_r(NULL,"=",&temp1),NULL,10);
               if(n==0)
               {
                  nvisit=-1;
               }
               else 
                  nvisit=n;
               visitaDir(ric[0]);
            }
            else if(dim==1)
            {
               nvisit=-1;
               visitaDir(ric[0]);
            }
            else
            {
               if(p==1)
                  printf("Errore: Parametri inseriti in maniera errata\n");
            }
         break;

         case 'W':   
            dim=parse_str(&ric,optarg,",");  
            CreateWrite(ric,dim);  
         break;

         case 'r':     
            dim=parse_str(&ric,optarg,",");         
            result=letturaFiles(ric,dim);
         break;

         case 'R':
            strtok_r(optarg,"=", &temp1);   
            int n=strtol(strtok_r(NULL,"=",&temp1),NULL,10);
            letturaRandom(n); 
         break;

         case 'a':
            dim=parse_str(&ric,optarg,",");
            if(dim>2)
            {
               printf("ERRORE:servono due argomenti <nomefile>,<filecontenuto>\n");
            }
            else
            {
               append(ric[0],ric[1]);
            }       
         break;
       
         case 'l':   
            dim=parse_str(&ric,optarg,",");
            for(int i=0;i<dim;i++)
            {
               result=openFile(ric[i],O_OPEN);
               if(t==1)
               {
                  usleep(1000*seconds);
               }
               if(result==1)
                  result=lockFile(ric[i]);
               if(t==1)
               {
                  usleep(1000*seconds);
               }
               if(result==1)
                  closeFile(ric[i]);
               if(p)
                  printf("\n");
            }  
         break;

         case 'u':     
            dim=parse_str(&ric,optarg,",");
            for(int i=0;i<dim;i++)
            {
               result=openFile(ric[i],O_OPEN);
               if(t==1)
               {
                  usleep(1000*seconds);
               }
               if(result==1)
                  result=unlockFile(ric[i]);
               if(t==1)
               {
                  usleep(1000*seconds);
               }
               if(result==1)
                  closeFile(ric[i]);
               if(p)
                  printf("\n");
            }
         break;

         case 'c':  
            dim=parse_str(&ric,optarg,",");  
            for(int i=0;i<dim;i++)
            {
               result=removeFile(ric[i]);
               if(p)
                  printf("\n");
            }
         break;

        
         case ':':     

         break;
      }
   }
   
   closeConnection(namesock);
   if(p==1)
      printf("--------------------TERMINATO--------------------\n\n\n\n");
   free(namesock);
   return 0;
}

// Stampa il comando help
void PrintCommand()
{
   printf("Comandi client:\n \
         -h: mostra i comandi.\n \
         -f filename: specifica il nome del socket AF_UNIX a cui connettersi.\n \
         -w dirname[,n=0]: invia tutti i file contenuti in dirname, ne invia al massimo n se n e' specificato.\n \
         -W file1[,file2]: invia i file passati come argomento.\n \
         -D dirname: permette di salvare in dirname i file espulsi in caso di capacity miss della cache. Da usare con -w o -W.\n \
         -r file1[,file2]: invia i file passati come argomento.\n \
         -d dirname: permette di salvare in dirname i file letti. Da usare con -r o -R.\n \
         -t time: imposta un intervallo tempo minimo tra una richiesta e l'altra.\n \
         -l file1[,file2] : lista di nomi di file su cui acquisire la mutua esclusione.\n \
         -u file1[,file2] : lista di nomi di file su cui rilasciare la mutua esclusione.\n \
         -c file1[,file2] : lista di file da rimuovere dal server se presenti.\n \
         -p abilita le stampe  dei risultati delle operazioni.\n \
         -a fileserver,filecontenuto : aggiorna il file <file server> scrivendo in append il contenuto di <filecontenuto>\n\n");
}

// funzione per creare un file e scriverlo
int CreateWrite(char** ric,int dim)
{
   int result1=0;
   for(int i=0;i<dim;i++)
   {
      result1=openFile(ric[i],O_CREATE);
      if(result1==1)
      {
         if(t==1)
         {
            usleep(1000*seconds);
         }
         if(D==1)
         {
            writeFile(ric[i],backupFolder);
         }
         else
         {
            writeFile(ric[i],NULL);
         }
         if(t==1)
         {
            usleep(1000*seconds);
         }
         closeFile(ric[i]);
      }
      if(t==1)
      {
         usleep(1000*seconds);
      }
      if(p==1)
         printf("\n");
   }
   
   return 1;
}

//procedura richiamta da visitaDir richiede la scrittura del file sul server
void writeF(char* pathname)
{
   int result=0;
   result=openFile(pathname,O_CREATE);
   if(result==1)
   {
      if(t==1)
      {
         usleep(1000*t);
      }
      if(D==1)
      {
         writeFile(pathname,backupFolder);
      }
      else
      {
         writeFile(pathname,NULL);
      }
      if(t==1)
      {
         usleep(1000*seconds);
      }
      closeFile(pathname);
      if(t==1)
      {
         usleep(1000*seconds);
      }
   }
   if(p==1)
      printf("\n");
}


//funzione utilizzata da visitaDir
int isdot(const char dir[]) 
{
  int l = strlen(dir);
  
  if ( (l>0 && dir[l-1] == '.') ) return 1;
  return 0;
}

/*funzione per -w visita in maniera ricorsiva la directory*/
void visitaDir(char* nomedir)
{
   struct stat statbuf;
   int r;
   int n=100;
   SYSCALL_EXIT(stat,r,stat(nomedir,&statbuf),"Facendo stat del nome %s: errno=%d\n",nomedir, errno);
   if(!S_ISDIR(statbuf.st_mode)) {printf("NON TROVA LA DIRECTORY\n");exit(EXIT_FAILURE);}
   DIR * dir;
   fprintf(stdout, "Directory %s:\n\n",nomedir);
   
   if ((dir=opendir(nomedir)) == NULL) 
   {
      perror("opendir");
      print_error("Errore aprendo la directory %s\n", nomedir);
      return;
   } else 
   {
	   struct dirent *file;
    
	   while((errno=0, file =readdir(dir)) != NULL && nvisit!=0) 
      {
         struct stat statbuf;
         char filename[n]; 
         int len1 = strlen(nomedir);
         int len2 = strlen(file->d_name);
         if ((len1+len2+2)>n) 
         {
            fprintf(stderr, "ERRORE: MAXFILENAME troppo piccolo\n");
            exit(EXIT_FAILURE);
         }	    
         strncpy(filename,nomedir,      n-1);
         strncat(filename,"/",         n-1);
         strncat(filename,file->d_name, n-1);
        
         if (stat(filename, &statbuf)==-1) 
         {
            perror("eseguendo la stat");
            print_error("Errore nel file %s\n", filename);
            return;
         }
          //Controllo per vedere se si tratta di una directory
         if(S_ISDIR(statbuf.st_mode)) {
            if ( !isdot(filename) ) 
               visitaDir(filename);
         } 
         else   
         { //se non è una directory
            nvisit=nvisit-1;
            writeF(filename);
         }
	   }
      if (errno != 0) perror("readdir");
      closedir(dir);
   }
}

//procedura chiamta da -r
int letturaFiles(char** ric, int dim)
{
   int result1=0;
   int result2=0;
   void * buf;
   size_t size;
   for(int i=0;i<dim;i++)
   {
      result1=openFile(ric[i],O_OPEN); //apro il file e basta
      if(t==1)
      {
         usleep(1000*seconds);
      }
      if(result1==1)
         result2=readFile(ric[i],&buf,&size);
      if(result2==1)
      {
         if(d==1)
         {
            struct stat statbuf;
            printf("\nDownload Folder: %s\n",downloadFolder);
            if(stat(downloadFolder, &statbuf)==-1) 
            {
               print_error("Errore: Directory di download non esistente %s\n", downloadFolder);
               return -1;
            }
            if(S_ISDIR(statbuf.st_mode)) 
            {
                  char* newdir=strc(downloadFolder,"/");
                  FILE* filed;
                  char *filename;
                  char** temp;    
                  char* files=malloc(sizeof(char)*strlen(ric[i]));
                  strcpy(files,ric[i]);
                  int result=parse_str(&temp,ric[i],"/");
                  filename=strc(newdir,temp[result-1]);
                  if((filed=fopen(filename,"wb"))==NULL)
                  {
                     print_error("Errore: Directory di download non esistente %s\n", downloadFolder);
                     return -1;
                  }
                  else
                  {
                     if(fwrite(buf,sizeof(char),size,filed)==0)
                     {
                        printf("ERRORE:  SCRITTURA\n");
                        return -1;
                     }
                     else
                     {
                        printf("File %s scritto correttamente nella cartella\n\n",filename);
                     }
                  }
                  free(temp);
                  free(filename);
                  free(buf); 
                  fclose(filed);
                  if(t==1)
                  {
                     usleep(1000*seconds);
                  }
                  closeFile(files);     
            }
            else
            {
               printf("ERROR:cartella non esistente\n");  
            }
         }
         else
         {
            if(t==1)
            {
               usleep(1000*seconds);
            }  
            closeFile(ric[i]);
            if(p==1)
               printf("\n");
         }
      }
      if(p==1)
         printf("\n");
   }
   return 1;
}

//funzione chiamata dall'opzione -a
int append(char* namefile,char* contentfile)
{
   int result=0;
   void* buffer;
   struct stat sb;
   int filed;
   if((filed=open(contentfile,O_RDONLY))==-1)
   {
      print_error("Errore: il file %s da aggiungere è inesitente\n",contentfile);
      return -1;
   }
   if(stat(contentfile, &sb) == -1) 
   { 
      print_error("Errore: il file %s da aggiungere è inesitente\n",contentfile);
     return -1;
   }
   buffer=malloc(sb.st_size);
   int dimfile=read(filed,buffer,sb.st_size);
   result=openFile(namefile,O_OPEN);
   if(result==1)
      appendToFile(namefile,buffer,dimfile,NULL);
   closeFile(namefile);
   if(p==1)
      printf("\n");
   free(buffer);
   return 1;
}

//procedura chamta da -R
void letturaRandom(int n)
{
   if(d==1)
   {
      readNFiles(n,downloadFolder);
   }
   else
   {
      readNFiles(n,NULL);
   }
}

//procedura per il controllo dei parametri
void controlFirstStr(char** argv,int argc)
{
   for(int i=1;i<argc;i++)
   {    

      if(strcmp(argv[i],"-f")==0)
      {      
         if(f==0)
         {
            f=1;     
         }
         else
         {
            printf("ERROR: l'opzione -f non può essere ripetuta\n");
            exit(EXIT_FAILURE);
         }
      }
      else if(strcmp(argv[i],"-p")==0)
      {
         if(p==0)
            p=1;
         else 
         {
            printf("ERROR: l'opzione -p non può essere ripetuta\n");
            exit(EXIT_FAILURE);
         }
      }
      else if(strcmp(argv[i],"-h")==0)
      {
         if(h==0)
         {   
          
            h=1;
            PrintCommand();
         }
         else
         {
            printf("ERROR: l'opzione -h non può essere ripetuta\n");
            exit(EXIT_FAILURE);
         }
      }
      
      else if(strcmp(argv[i],"-d")==0)
      {
         d=1;      
         downloadFolder=malloc(sizeof(char)*strlen(argv[i+1])+1);
         strcpy(downloadFolder,argv[i+1]);
        
      }
      else if(strcmp(argv[i],"-D")==0)
      {
         D=1;
         backupFolder=malloc(sizeof(char)*strlen(argv[i+1])+1);
         strcpy(backupFolder,argv[i+1]);
      } 
      else if (strcmp(argv[i],"-W")==0)
      {
         W=1;
      }
      else if(strcmp(argv[i],"-w")==0)
      {
         w=1;
      }
       else if(strcmp(argv[i],"-r")==0)
      {
         r=1;
      }
       else if(strcmp(argv[i],"-R")==0)
      {
         R=1;
      }
      else if(strcmp(argv[i],"-t")==0)
      {
         seconds=strtol(argv[i+1],NULL,10);
         t=1;  
      }
   }
      if(f==0)
      {
         printf("Error: Devi Specificare il nome della Socket\n");
         exit(EXIT_FAILURE);
      }
      if( D==1 && W==0 && w==0)
      {
         printf("ERROR:-D deve essere usata congiuntamente all'opzione -W o -w\n");
         exit(EXIT_FAILURE);
      }
      if( d==1 && r==0 && R==0)
      {
          printf("ERROR:-d deve essere usata congiuntamente all'opzione -R o -r\n");
          exit(EXIT_FAILURE);
      }
      
}
