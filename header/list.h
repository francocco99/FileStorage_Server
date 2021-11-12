
typedef struct nod_e
{
   char* key;
   void* cont;
   struct nod_e *next;
   struct nod_e *prev;
}node;

typedef struct list
{
   int length;
   node * header;
   node *tailer;
}list;
typedef list* Lis;
Lis create();
void insertH(list *l,void *cont,char *k);
void insertT(list *l,void *cont,char *k);
node* UpdateNode(list *l, char*k,void* cont);
node* createnode(void* value,char *key, node* next, node* prev);
node* search(list *list, char *k);
node* delete(list *fs, char *k);
node* takeHead(list *l);
int isEmpty(Lis l);
void stampa(Lis l);

