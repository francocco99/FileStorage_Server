#include <stdio.h>
#include <stdio.h>
#include <string.h>
#include "../header/hash.h"

/*hash function djb2*/
int funchash(char* key, int maxsize)
{
   unsigned long hash = 5381;
    int c;

    while ((c = *key++))
        hash = ((hash << 5) + hash) + c; 

    return hash % maxsize;
}

hash* h_create(int size)
{
   hash* Hash=malloc(sizeof(hash));
   list** buck=malloc(sizeof(list *)*size);
   Hash->size=size;
   Hash->nelem=0;
   Hash->buckets=buck;
   
   for(int i=0;i<size;i++)
   {
      Hash->buckets[i]=NULL;
   }
   return Hash;
}

int h_delete(hash** hash,char* key)
{
  
   int pos=funchash(key,(*hash)->size);
   Lis current=(*hash)->buckets[pos];
   if(delete(current,key)==NULL) return 0;
   else
   {
      (*hash)->nelem=(*hash)->nelem-1;
      return 1;

   } 
}

int h_update(hash** hash,char* key,void* newvalue)
{
   int pos=funchash(key,(*hash)->size);
   Lis current=(*hash)->buckets[pos];
   if(UpdateNode(current,key,newvalue)==NULL)
   {
      return 0;
   }
   else
   {
      return 1;
   }
}

int hashins(hash** hash,char* key, void* value)
{
   
   if(hash==NULL)
   {
      fprintf(stderr,"ERROR");
      exit(EXIT_FAILURE);
   }
      
      int k=funchash(key,(*hash)->size);
      // crea la lista associata a quella posizione
      if((*hash)->buckets[k]==NULL)
      {
         (*hash)->buckets[k]=create();
      }
         (*hash)->nelem=(*hash)->nelem+1;
         insertH((*hash)->buckets[k],value,key);
         
 
        return 1;
}
int  contain(hash* h,char* key)
{
   int pos=funchash(key,h->size);
   Lis l=h->buckets[pos];
   if(l==NULL) return 0;
   if(search(l,key)!=NULL) return 1;
   else return 0;
}
void* getvalue(hash* h,char*key)
{
   int pos=funchash(key,h->size);
   Lis l=h->buckets[pos];
   node* f;
   if(l==NULL) return NULL;
   else 
   {
     if((f=search(l,key))!=NULL)
     {
        return f->cont;
     }
   }
   return NULL;
}

