
typedef struct nod_e
{
   const char* pathname;
   void* content;
   struct nod_e *next;
   struct nod_e *prev;
}node;

node* create(const char* pathname,void* conent);
void destroy(const char* pathname)