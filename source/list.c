#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../header/list.h"
#define DIMK 50

Lis create()
{
   Lis l=malloc(sizeof(list));
   l->header=NULL;
   l->tailer=NULL;
   l->length=0;
   return l;
}

node* createnode(void* value,char* key,node* next, node* prev)
{
   node* new=malloc(sizeof(node));
   new->key=malloc(sizeof(char)*DIMK);
   strcpy(new->key,key);
   new->cont=value;
   new->next=next;
   new->prev=prev;
   return new;

}
//CONTROLLARE
void insertH(list *l,void *cont,char *k)
{
   
   node* new=createnode(cont,k,l->header,NULL);
   if(l->length==0)
   {  
      l->tailer=new;
   }
   else   
   {
      l->header->prev=new;
   }
   l->header=new;
   l->length++;
}
//CONTROLLARE
void insertT(list *l, void *cont, char* k)
{
   
   node* new=createnode(cont,k,NULL,l->tailer);
   if(l->tailer==NULL)
   {
      l->header=new;
   }
   else
   {
      l->tailer->next=new;
   }
   l->tailer=new;
   l->length++;
   

}
node* UpdateNode(list *l, char*k,void* cont)
{
   node *head=l->header;
   node *tail=l->tailer;
   if(head==NULL)return NULL;
   if(strcmp(head->key,k)==0)
   {
      head->cont=cont;
       return head;
   }
   if(strcmp(tail->key,k)==0)
   {
      tail->cont=cont;
      return tail;
   }
   while(head!=NULL && strcmp(head->key,k)!=0)
   {
      head=head->next;
   }
   if(head!=NULL)
   {
      head->cont=cont;
       return head;
   }
   else
   {
      return NULL;
   }
}
node* delete(list *l, char* k)
{
   node *tmp;
   tmp=l->header;
   if(strcmp(l->header->key,k)==0)
   {
      l->header=l->header->next;
      if(l->header!=NULL)
      {
         l->header->prev=NULL;
      }
      l->length--;
      free(tmp);
      return tmp;
   }
      
   while(tmp!=NULL)
   {   
       if (strcmp(tmp->key,k)==0)
        {
          
            tmp->prev->next = tmp->next;
            if(tmp->next !=NULL)
               tmp->next->prev=tmp->prev;
            
            free(tmp);
            l->length--;
            return tmp;
        }
        tmp = tmp->next;
   }
   return NULL;

}
node* search(list *l, char *k)
{
   node *head=l->header;
   node *tail=l->tailer;
   if(head==NULL)return NULL;
   if(strcmp(head->key,k)==0)
   {
      return head;
   }
   if(strcmp(tail->key,k)==0)
   {
      return tail;
   }
   while(head!=NULL && strcmp(head->key,k)!=0)
   {
      head=head->next;
   }
   if(head!=NULL)
   {
      return head;
   }
   else
   {
      return NULL;
   }
}
node* takeHead(list *l)
{
   node* tmp;
   tmp=l->header;
   l->header=l->header->next;
   if(l->header!=NULL)
   {
      l->header->prev=NULL;
   }  
   
   l->length--;
   if(l->length==0)
   {
      l->header=NULL;
      l->tailer=NULL;
   }
  
   //c'è da fare la free
   return tmp;

}
int isEmpty(Lis l)
{
   if(l->length==0)return 1;
   else return 0;
}
void stampa(Lis l)
{
   node* temp=l->header;
   while(temp!=NULL)
   {
      printf("%s->",temp->key);
      temp=temp->next;
   }
}
