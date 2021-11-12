  #include <stdlib.h>
#include "list.h"
typedef struct hash_t{
   int size; // dimensione
   int nelem;// numero elementi
   int collision;
   list** buckets; // array di liste
}hash;

// inserimento di un elemento con chiave k
int hashins(hash** hash, char* key, void* value);
void* getvalue(hash* hash,char*key);
// creazione di una tabella hash di size posizioni
hash* h_create(int size);
int h_delete(hash** hash,char* key);
int h_update(hash** hash,char* key,void* newvalue);
int funchash(char* key, int maxsize);
int contain();
int deletehash(hash** hash);
