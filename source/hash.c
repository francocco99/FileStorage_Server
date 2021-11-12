#include <stdio.h>
#include <stdio.h>
#include <string.h>
#include "../header/hash.h"

int funchash(char* key, int maxsize)
{
   //////DA CAMBIAREEEEEEEEEEEEEEEEEE
   int hash, i;
    int len = strlen(key);
    
    for (hash = i = 0; i < len; i++) {
        hash += key[i];
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);

    hash %= maxsize;
    if (hash < 0)
        hash += maxsize;
    return hash;
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
      printf("%d\n",k);
      // crea la lista associata a quella posizione
      if((*hash)->buckets[k]==NULL)
      {
         (*hash)->buckets[k]=create();
      }
         (*hash)->nelem=(*hash)->nelem+1;
         insertT((*hash)->buckets[k],value,key);
      
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

/*int main(int argc, char** argv)
{
   hash *H;
   H=h_create(10);
   hashins(&H,"ciao","3");
   hashins(&H,"ciao1","4");
   hashins(&H,"ciao","6");
   hashins(&H,"ciao2","76");
   hashins(&H,"ciao3","6");
   hashins(&H,"ciao5","1");
   hashins(&H,"ciao6","7");
   hashins(&H,"ciao1","9");
   hashins(&H,"ciao2","13");
   hashins(&H,"ciao4","45454");

   Lis rit;
   rit=create();

   for(int i=0;i<10;i++ )
   {
      Lis curr=H->buckets[i];
      if(curr!=NULL)
      {
         if(curr->length>0)
         {
            node* cur=curr->header;
            while(cur!=NULL)
            {
               printf("%s->",(char*)cur->cont);
               cur=cur->next;
            }
            printf("\n");
         }
      }
   }
      h_update(&H,"ciao1","14");
      h_update(&H,"ciao2","11");
      
   for(int i=0;i<10;i++ )
   {
      Lis curr=H->buckets[i];
      if(curr!=NULL)
      {
         if(curr->length>0)
         {
            node* cur=curr->header;
            while(cur!=NULL)
            {
               printf("%s->",(char*)cur->cont);
               cur=cur->next;
            }
            printf("\n");
         }
      }
     

   }
    printf("%d\n",contain(H,"ciao"));
    printf("%d\n",contain(H,"cazzo"));

}*/
