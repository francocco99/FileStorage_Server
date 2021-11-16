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
#define SOCKNAME "/mysock"
#define k 20
#define N 50
void PrintCommand();
void inviames();
int  CreateWrite(char** ric,int dim);//metodo per creare un file e scriverlo 
void visitaDir(char* nomedir,int n); // metodo per 
void letturaFiles(char** ric, int dim);
void letturaRandom(int n);
int isdot(const char dir[]);
void lsR(const char nomedir[]);
void writeF(char* pathname);
void controlFirstStr(char** argv, int argc);
void append(char* namefile,char* contentfile);

char operation[N];
char backupFolder[N];
char* downloadFolder;
char prova[100];
int result=0;
//FLAG PER IL CONTROLLO DEGLI ARGOMENTI
int f=0;
int p=0;
int h=0;
int D=0;
int d=0;
int t=0;
int w,W=0;
int r,R=0;
int seconds=0;
int main(int argc, char* argv[])
{
   ///PER LA OPEN
   int sec= 1000; 
   struct timespec abs;
   clock_gettime(CLOCK_REALTIME,&abs); 
   abs.tv_sec=abs.tv_sec+10;
   //prendo l'orario corrente  e ci sommo 20 secondi
   //////
   int FlagP; //se uguale ad 1 tutte le stampe sono abilitate  
   int opt;  
   char*sockname=malloc(sizeof(char)*k);
   char* buf=malloc(sizeof(char)*N);
   char* namesock=malloc(sizeof(char)*N);

    controlFirstStr(argv,argc);
   
 
   //inserire i controlli per la gestione dei comandi

  // se il primo carattere della optstring e' ':' allora getopt ritorna
  // ':' qualora non ci sia l'argomento per le opzioni che lo richiedono
  // se incontra una opzione (cioe' un argomento che inizia con '-') e tale
  // opzione non e' in optstring, allora getopt ritorna '?'
   while ((opt = getopt(argc,argv, ":a:h:f:w:W:D:r:R:d:t:l:u:c:p")) != -1) 
   {  
      int dim;
      char** ric;
      char* temp1;
      switch(opt) 
      {
        
         case 'f':   
            
            strcpy(namesock,optarg);   
            if(openConnection(optarg,sec,abs)==-1)
            {
               printf("impossibile connettersi\n");
               exit(EXIT_FAILURE);
            }
         break;
         case 'p': 
               abilitastampe();          
         break;

         case 'w':     
            dim=parse_str(&ric,optarg,",");
            if(dim>2)
            {
               printf("ERRORE\n");
            }
            else
            {
               char* temp1;
               strtok_r(ric[1],"=", &temp1);   
               int n=strtol(strtok_r(NULL,"=",&temp1),NULL,10);
               visitaDir(ric[0],n);
            }
         break;
         case 'W':   
            dim=parse_str(&ric,optarg,",");  
            printf("%d\n",dim);
            CreateWrite(ric,dim);  
         break;

         case 'r':     
            dim=parse_str(&ric,optarg,",");         
            letturaFiles(ric,dim);
   
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
               //controllo il file del contenuto
               
               
         break;
       
         case 'l':   
               dim=parse_str(&ric,optarg,",");

               for(int i=0;i<dim;i++)
               {
                  result=lockFile(ric[i]);
               }  

         break;
         case 'u':     
               dim=parse_str(&ric,optarg,",");
         
               for(int i=0;i<dim;i++)
               {
                  result=unlockFile(ric[i]);
               }
         
         break;
         case 'c':  
               dim=parse_str(&ric,optarg,",");
              
               for(int i=0;i<dim;i++)
               {
                  result=removeFile(ric[i]);
               }

            
         
         break;
        
         case ':':     break;
      }
      

   }

   // per communicare con il server
  
 //  closeConnection(namesock);
   return 0;
}

// funzione che stampa i comandi  possibili(comando help)
void PrintCommand()
{

}
int CreateWrite(char** ric,int dim)
{
   int result1=0;
   int result2=0;
   for(int i=0;i<dim;i++)
   {
      result1=openFile(ric[i],O_CREATE);
      if(result1==1)
      {
         if(t==1){usleep(1000*seconds);}
         if(D==1)
         {
            result2=writeFile(ric[i],backupFolder);
         }
         else
         {
            result2=writeFile(ric[i],NULL);
            //sposto close file 30/10/2021
         }
         
      }
      if(t==1){usleep(1000*seconds);}
      closeFile(ric[i]);


   }
}
void writeF(char* pathname)
{

   openFile(pathname,O_CREATE);
   if(t==1){usleep(1000*t);}
   if(D==1)
   {
      writeFile(pathname,backupFolder);
   }
   else
   {
      writeFile(pathname,NULL);
   }
   if(t==1){usleep(1000*seconds);}
   closeFile(pathname);
   printf("%s\n",pathname);

}
int isdot(const char dir[]) {
  int l = strlen(dir);
  
  if ( (l>0 && dir[l-1] == '.') ) return 1;
  return 0;
}

void visitaDir(char* nomedir,int n)
{
    struct stat statbuf;
   int r;
   SYSCALL_EXIT(stat,r,stat(nomedir,&statbuf),"Facendo stat del nome %s: errno=%d\n",nomedir, errno);
   if(!S_ISDIR(statbuf.st_mode)) {printf("NON TROVA LA DIRECTORY\n");exit(EXIT_FAILURE);}
   DIR * dir;
   fprintf(stdout, "-----------------------\n");
   fprintf(stdout, "Directory %s:\n",nomedir);
   
   if ((dir=opendir(nomedir)) == NULL) 
   {
      perror("opendir");
      print_error("Errore aprendo la directory %s\n", nomedir);
      return;
   } else 
   {
	   struct dirent *file;
    
	   while((errno=0, file =readdir(dir)) != NULL && n>0) 
      {
         struct stat statbuf;
         char filename[N]; 
         int len1 = strlen(nomedir);
         int len2 = strlen(file->d_name);
         if ((len1+len2+2)>N) 
         {
            fprintf(stderr, "ERRORE: MAXFILENAME troppo piccolo\n");
            exit(EXIT_FAILURE);
         }	    
         strncpy(filename,nomedir,      N-1);
         strncat(filename,"/",         N-1);
         strncat(filename,file->d_name, N-1);
        
         if (stat(filename, &statbuf)==-1) 
         {
            perror("eseguendo la stat");
            print_error("Errore nel file %s\n", filename);
            return;
         }
          //CONTROLLO PER VEDERE SE SI TRATTA DI UNA DIRECTORY
         if(S_ISDIR(statbuf.st_mode)) {
            if ( !isdot(filename) ) 
               visitaDir(filename,n);
            } 
            else   
            { //se non è una directory
               n=n-1;
               writeF(filename);
            }
	   }
      if (errno != 0) perror("readdir");
      closedir(dir);
      fprintf(stdout, "-----------------------\n");
   }
}

void letturaFiles(char** ric, int dim)
{
   int result1=0;
   int result2=0;
   void * buf;
   size_t size;
   for(int i=0;i<dim;i++)
   {
      result1=openFile(ric[i],0);
      if(t==1){usleep(1000*seconds);}
      result2=readFile(ric[i],&buf,&size);
      if(result2==1)
      {
         if(d==1)
         {
            ///DA RICONTROLLARE
            struct stat statbuf;
            printf("CARTELLA DOVE VADO A SCRIVERE IL FILE%s\n",downloadFolder);
            if(stat(downloadFolder, &statbuf)==-1) 
            {
               perror("eseguendo la stat");
               print_error("Errore nel file %s\n", downloadFolder);
               return;
            }
            if(S_ISDIR(statbuf.st_mode)) {
                  
                  FILE* filed;
                  char *filename;
                  char** temp;    
                  char* files=malloc(sizeof(char)*strlen(ric[i]));
                  strcpy(files,ric[i]);
                  int result=parse_str(&temp,ric[i],"/");
                  filename=strc(downloadFolder,temp[result-1]);
                  if((filed=fopen(filename,"wb"))==NULL)
                  {
                     perror("file.txt, in apertura");
                     exit(EXIT_FAILURE);
                  }
                  else
                  {
                     if(fwrite(buf,sizeof(char),size,filed)==0)
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
                  free(buf); //inserito dopo forse errore
                  fclose(filed);
                  printf("%s\n",files);
                  closeFile(files);
             }
             else
             {
                printf("ERROR:cartella non esistente\n");
                
             }

         }
         else
         {
            closeFile(ric[i]);
         }
         
      }

   }
}

void append(char* namefile,char* contentfile)
{
   void* buffer;
   struct stat sb;
   int filed;
   if((filed=open(contentfile,O_RDONLY))==-1)
   {
      perror("ERRORE: in apertura");
      exit(EXIT_FAILURE);
   }
   if(stat(contentfile, &sb) == -1) { // impossibile aprire statistiche file
      errno = EBADF;
      return -1;
   }
   buffer=malloc(sb.st_size);
   int dimfile=read(filed,buffer,sb.st_size);
   openFile(namefile,OPEN); //CONTROLLARE SE FUNZIONA
   appendToFile(namefile,buffer,dimfile,NULL);
   closeFile(namefile);
   free(buffer);
}

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

void controlFirstStr(char** argv,int argc)
{
   for(int i=1;i<argc;i++)
   {    
      //CONTROLLI
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
               h=1;
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
