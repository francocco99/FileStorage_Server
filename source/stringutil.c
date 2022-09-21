#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "../header/stringutil.h"
#define N 30

//funzione che prende una richiesta e fa il parsing in base ad un o specifico carattere
int parse_str(char*** ris,char* ric, const char* delim)
{ 
      *ris=malloc(sizeof(char *)*N);
      int i=0;
      char *tmpstr;
      char *token = strtok_r(ric, delim, &tmpstr);
      while (token) 
      {
         (*ris)[i]=malloc(sizeof(char)*(strlen(token)+1));
    
         strcpy((*ris)[i],token);
         i++;
         if(i==N){
            ris=realloc(*ris,(2*N)*sizeof(char*)); 
            }
         token = strtok_r(NULL, delim, &tmpstr);
      } 
      
      return i;
      
}

// funzione che prende due strinche e le concatena tra loro
char * strc(const char* s1,char* s2)
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
