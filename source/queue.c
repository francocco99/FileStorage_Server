#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "../header/queue.h"



// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// CREATE OR DELETE A NODE

Node* Node_create(void* element, Node* next, Node* prev) {
    Node* newNode = malloc(sizeof(*newNode));
    assert(newNode);

    newNode->element = element;
    newNode->next = next;
    newNode->prev = prev;

    return newNode;
}

void Node_free(Node** nodePtr) {
    assert(nodePtr);
    free(*nodePtr);
    *nodePtr = NULL;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// CREATE - DELETE A LIST

List List_create() {
    List newList = malloc(sizeof(*newList));
    assert(newList);

    newList->length = 0;
    newList->head = NULL;
    newList->tail = NULL;

    return newList;
}

void List__free(Node** nodePtr) {
    if (*nodePtr != NULL) {
        List__free(&(*nodePtr)->next);
        Node_free(nodePtr);
    }
}

void List_free(List* listPtr) {
    List__free(&(*listPtr)->head);
    free(*listPtr);
    *listPtr = NULL;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

// INSERT INTO THE TAIL

void List_insertTail(List list, void* element) {  // O(1)
    Node* newNode = Node_create(element, NULL, list->tail);

    if (list->tail != NULL) {  // list->tail == NULL <=> list.length == 0
        list->tail->next = newNode;
    } else {
        list->head = newNode;
    }

    list->tail = newNode;

    list->length++;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// DELETE HEAD

void List_deleteHead(List list) {  // O(1)
    if (list->head == NULL) {      // empty list
        return;
    }

    size_t listLength = List_length(list);
    if (listLength == 1) {
        list->tail = NULL;
    }

    Node* nodeToRemove = list->head;
    list->head = list->head->next;
    if (list->head != NULL) {
        list->head->prev = NULL;
    }
    Node_free(&nodeToRemove);

    list->length--;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// PRINT

void List_print(List list) {
    Node* node = list->head;
    while (node != list->tail) {
       //printf("%d -> ", node->element->);
        node = node->next;
    }
    if (list->tail != NULL) {
       // printf("%d\n", list->tail->element);
    }
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// GET THE LENGTH

size_t List_length(List list) {
    return list->length;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// GET A REFERENCE TO A NODE BY ITS INDEX

Node* List__getNodeByIndex(Node* node, int index, int counter) {
    if (node == NULL) {
        return NULL;
    }
    if (counter == index) {
        return node;
    } else {
        return List__getNodeByIndex(node->next, index, counter + 1);
    }
}

Node* List_getNodeByIndex(List list, int index) {
    if (index < 0) {
        return NULL;
    }
    return List__getNodeByIndex(list->head, index, 0);
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// CREATE DELETE A QUEUE

Queue* Queue_create() {
    Queue* queue = malloc(sizeof(*queue));
    assert(queue);
    queue->queue = List_create();
    assert(queue->queue);
    return queue;
}

void Queue_delete(Queue** queuePtr) {
    List_free(&(*queuePtr)->queue);
    free(*queuePtr);
    *queuePtr = NULL;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// ENQUEUE - DEQUEUE

void Queue_enqueue(Queue* queue, void* element) {
    return List_insertTail(queue->queue, element);
}

void* Queue_dequeue(Queue* queue) {
    assert(Queue_length(queue) > 0);
    Node* node = List_getNodeByIndex(queue->queue, 0);
    void* element = node->element;
    List_deleteHead(queue->queue);
    return element;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// QUEUE LENGTH

int Queue_length(Queue* queue) {
    return List_length(queue->queue);
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// QUEUE PRINT

void Queue_print(Queue* queue) {
    return List_print(queue->queue);
}
int Queue_isempty(Queue* queue)
{
   if(Queue_length(queue)==0)
      return 1;
   else
      return 0;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

