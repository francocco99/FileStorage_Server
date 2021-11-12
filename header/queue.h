#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "Protcol.h"

typedef struct Node {
    void* element;
    struct Node* next;
    struct Node* prev;
} Node;

typedef struct ListEntity {
    Node* head;
    Node* tail;
    size_t length;
} ListEntity;

typedef ListEntity* List;

typedef struct Queue {
    List queue;
} Queue;

// node
Node* Node_create(void* element, Node*, Node*);
void Node_free(Node**);

// list
List List_create();
void List_free(List*);
void List_deleteHead(List);
void List_insertTail(List, void*);
size_t List_length(List);
void List_print(List);
Node* List_getNodeByIndex(List, int);
Node* List__getNodeByIndex(Node*, int, int);

// queue
Queue* Queue_create(void);
void Queue_delete(Queue**);
void Queue_enqueue(Queue*, void*);
void* Queue_dequeue(Queue*);
int Queue_length(Queue*);
int Queue_isempty(Queue*);
void Queue_print(Queue*);
