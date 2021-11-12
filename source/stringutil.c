#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "../header/stringutil.h"
#define N 20
//funzione che prende una richiesta e fa il parsing in base ad un o specifico carattere

//PROBLEMA NON FUNZIONA PER >10
int parse_str(char*** ris,char* ric, const char* delim)
{ 
      *ris=malloc(sizeof(char *)*N);
      int i=0;
      char *tmpstr;
      char *token = strtok_r(ric, delim, &tmpstr);
      while (token) 
      {
         (*ris)[i]=malloc(sizeof(char)*strlen(token));
    
         strcpy((*ris)[i],token);
         i++;
         if(i==N){
            realloc(*ris,(2*N)*sizeof(char*)); printf("QUA CI ENTRO\n");
            }
         token = strtok_r(NULL, delim, &tmpstr);
      } 
      
      return i;
      
}
// funzione che prende due strinche e le concatena tra loro
char * strc(char* s1,char* s2)
{  
   char* concats=malloc(sizeof(char)*(strlen(s1)+strlen(s2)+1));
   strncpy(concats,s1,strlen(s1));
   concats[strlen(s1)]='\0';
   strcat(concats,s2);
   return concats;
}
// funzione che prende una o più stringhe e le concatena tra loro
char * concat(char* str, ...)
{
   va_list ap;
   va_start(ap,str);
   char *res;
   char *s=NULL;
   while((s=va_arg(ap, char*))!=NULL)
   {
      res=strc(str,s);
      str=res;  
   }
   va_end(ap);  
   return res;
}
/*int main(int argc, char *argv[])
{
  /* int n=0;
   char** k=NULL;
   char ric[30];
   scanf("%s",ric);
   n=parse_str(&k,ric,",");
   printf("%d\n",n);
   if(k==NULL){printf("male");return 0;}
   printf("%s\n",k[1]);
   for(int i=0;i<n;i++)
   {
      printf("%s\n",k[i]);
   }
   char  prova[1000];
   strcpy(prova,concat("ciao",":","fallito","chedici", "coglione",NULL));
   printf("%s\n",prova);
}*/