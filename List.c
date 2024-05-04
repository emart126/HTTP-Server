//--------------------------------
// Eric Martinez, CruzID:emart126
// List.c
//--------------------------------

#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "List.h"
#include "rwlock.h"

// Helper Functions -----------------------------------------------------------
void copyStr(int len, char str[], char sub[]) {
    for (int i = 0; i < len; i++) {
        sub[i] = str[i];
    }
    sub[len] = '\0';
    return;
}

// Structs --------------------------------------------------------------------

// private Node type
typedef struct NodeObj *Node;

// private NodeObj type
typedef struct NodeObj {
    listElement count;
    char uri[65];
    rwlock_t *rw;
    Node next;
    Node prev;
} NodeObj;

// private listObj type
typedef struct listObj {
    Node front;
    Node back;
    Node cursor;
    int cIndex;
    int length;
    pthread_mutex_t listLock;
} listObj;

// Constructors-Destructors ---------------------------------------------------

// newNode()
// Returns reference to new Node object. Initialized next and data fields.
Node newNode(listElement initialCount, char newURI[]) {
    Node N = malloc(sizeof(NodeObj));
    assert(N != NULL);
    N->count = initialCount;
    N->rw = rwlock_new(N_WAY, 1);
    copyStr(strlen(newURI), newURI, N->uri);
    N->next = NULL;
    N->prev = NULL;
    return (N);
}

// freeNode()
// Frees heap memory pointed to by *pN, sets *pN to NULL.
// Pre: pN != NULL and value in pN != NULL
void freeNode(Node *pN) {
    if (pN != NULL && *pN != NULL) {
        rwlock_delete(&(*pN)->rw);
        free(*pN);
        *pN = NULL;
    }
}

// newList()
// Creates and returns a new empty List.
List newList(void) {
    List L;
    L = malloc(sizeof(listObj));
    assert(L != NULL);
    L->front = NULL;
    L->back = NULL;
    L->cursor = NULL;
    L->cIndex = -1;
    L->length = 0;
    pthread_mutex_init(&(L->listLock), NULL);
    return (L);
}

// freeList()
// Frees all heap memory associated with *pL, and sets *pL to NULL.
// Pre: pL != NULL and value in pL != NULL
void freeList(List *pL) {
    if (pL != NULL && *pL != NULL) {
        while ((*pL)->length != 0) {
            deleteFront(*pL);
        }
        pthread_mutex_destroy(&((*pL)->listLock));
        free(*pL);
        *pL = NULL;
    }
}

// Access functions -----------------------------------------------------------

// length()
// Returns the number of elements in L.
// Pre: L!=NULL
int length(List L) {
    return L->length;
}

// lIndex()
// Returns index of cursor element if defined, -1 otherwise.
// Pre:
int lIndex(List L) {
    return L->cIndex;
}

// front()
// Returns front element of L
// Pre: length()>0
int front(List L) {
    if (length(L) <= 0) {
        fprintf(stderr, "List Error: calling front() on list length of zero\n");
        exit(1);
    }

    return L->front->count;
}

// back()
// Returns back element of L
// Pre: length()>0
int back(List L) {
    if (length(L) <= 0) {
        fprintf(stderr, "List Error: calling back() on list length of zero\n");
        exit(1);
    }

    return L->back->count;
}

// get()
// Returns cursor element of L
// Pre: length()>0, index()>=0
int get(List L) {
    if (length(L) <= 0) {
        fprintf(stderr, "List Error: calling get() on list length of zero\n");
        exit(1);
    }
    if (lIndex(L) < 0) {
        fprintf(stderr, "List Error: calling get() on undefined cursor index\n");
        exit(1);
    }

    return L->cursor->count;
}

// Manipulation procedures ----------------------------------------------------

// clear()
// Resets L to its original empty state.
// Pre: L is non empty
void clear(List L) {
    L->cursor = NULL;
    L->cIndex = -1;
    while (L->length != 0) {
        deleteFront(L);
    }
}

// set()
// Overwrites the cursor element’s data with x.
// Pre: length()>0, index()>=0
void set(List L, int x) {
    if (length(L) <= 0) {
        fprintf(stderr, "List Error: calling set() on list length of zero\n");
        exit(1);
    }
    if (lIndex(L) < 0) {
        fprintf(stderr, "List Error: calling set() on undefined cursor index\n");
        exit(1);
    }

    L->cursor->count = x;
}

// moveFront()
// If L is non-empty, sets cursor under the front element, otherwise does nothing.
// Pre: L != NULL
void moveFront(List L) {
    if (L->length != 0) {
        L->cursor = L->front;
        L->cIndex = 0;
    }
}

// moveBack()
// If L is non-empty, sets cursor under the back element, otherwise does nothing.
// Pre: L != NULL
void moveBack(List L) {
    if (L->length != 0) {
        L->cursor = L->back;
        L->cIndex = (L->length - 1);
    }
}

// movePrev()
// If cursor is defined and not at front, move cursor one
// step toward the front of L; if cursor is defined and at front,
// cursor becomes undefined; if cursor is undefined do nothing
// Pre: cursor is defined
void movePrev(List L) {
    if (L->cIndex != -1 && L->cursor->prev != NULL) {
        L->cursor = L->cursor->prev;
        L->cIndex -= 1;
    } else if (L->cIndex != -1 && L->cursor->prev == NULL) {
        L->cursor = NULL;
        L->cIndex = -1;
    }
}

// moveNext()
// If cursor is defined and not at back, move cursor one
// step toward the back of L; if cursor is defined and at back,
// cursor becomes undefined; if cursor is undefined do nothing
// Pre: cursor is defined
void moveNext(List L) {
    if (L->cIndex != -1 && L->cursor->next != NULL) {
        L->cursor = L->cursor->next;
        L->cIndex += 1;
    } else if (L->cIndex != -1 && L->cursor->next == NULL) {
        L->cursor = NULL;
        L->cIndex = -1;
    }
}

// append()
// Insert new element into L. If L is non-empty,
// insertion takes place after back element.
// Pre: L isn't NULL
void append(List L, int x, char uri[]) {
    if (L == NULL) {
        fprintf(stderr, "List Error: calling append() on NULL List\n");
        exit(1);
    }

    Node n = newNode(x, uri);
    if (L->length == 0) {
        L->front = n;
        L->back = n;
    } else {
        // insert n after back
        L->back->next = n;
        n->prev = L->back;
        L->back = n;
    }
    L->length += 1;
}

// deleteFront()
// Delete the front element.
// Pre: length()>0
void deleteFront(List L) {
    if (L->length == 0) {
        fprintf(stderr, "List Error: calling deleteFront() on list length of zero\n");
        exit(1);
    }

    if (L->length == 1) {
        freeNode(&(L->front));
        L->front = NULL;
        L->back = NULL;
        L->cursor = NULL;
        L->cIndex = -1;
    } else {
        L->front = L->front->next;
        freeNode(&(L->front->prev));
        L->front->prev = NULL;
        if (L->cIndex == 0) {
            L->cIndex = -1;
        } else {
            L->cIndex -= 1;
        }
    }
    L->length -= 1;
}

// deleteBack()
// Delete the back element.
// Pre: length()>0
void deleteBack(List L) {
    if (L->length == 0) {
        fprintf(stderr, "List Error: calling deleteBack() on list length of zero\n");
        exit(1);
    }

    if (L->cIndex == length(L) - 1) {
        L->cursor = NULL;
        L->cIndex = -1;
    }
    if (L->length == 1) {
        freeNode(&(L->front));
        L->front = NULL;
        L->back = NULL;
    } else {
        L->back = L->back->prev;
        freeNode(&(L->back->next));
        L->back->next = NULL;
    }
    L->length -= 1;
}

// delete()
// Delete cursor element, making cursor undefined.
// Pre: length()>0, index()>=0
void delete (List L) {
    if (L->length == 0) {
        fprintf(stderr, "List Error: calling delete() on list length of zero\n");
        exit(1);
    }
    if (lIndex(L) < 0) {
        fprintf(stderr, "List Error: calling delete() on undefined cursor index\n");
        exit(1);
    }

    Node c = NULL;
    c = L->cursor;
    if (c->prev == NULL) {
        deleteFront(L);
    } else if (c->next == NULL) {
        deleteBack(L);
    } else {
        c->prev->next = c->next;
        c->next->prev = c->prev;
        freeNode(&c);
        L->length -= 1;
        L->cIndex = -1;
    }
}

// Other operations -----------------------------------------------------------

// Lock List for use
void lockList(List L) {
    pthread_mutex_lock(&(L->listLock));
}

// Unlock list to allow others to use
void unlockList(List L) {
    pthread_mutex_unlock(&(L->listLock));
}

// searchList()
// searches list for str, returns true if it exists
int searchList(List L, char str[]) {
    moveFront(L);
    char *currNode;
    while (lIndex(L) != -1) {
        currNode = L->cursor->uri;
        if (strcmp(str, currNode) == 0) {
            return 1;
        }
        moveNext(L);
    }
    return 0;
}

// incrementURI()
// appends new node with 1 and with uri
// if it exists, increments count by 1
void incrementURI(List L, char uri[]) {
    lockList(L);
    moveFront(L);
    char *currNode;
    bool isInList = false;
    while (lIndex(L) != -1) {
        currNode = L->cursor->uri;
        if (strcmp(uri, currNode) == 0) {
            L->cursor->count += 1;
            isInList = true;
            break;
        }
        moveNext(L);
    }
    if (!isInList) {
        append(L, 1, uri);
    }
    unlockList(L);
}

// decrementURI()
// decrement str count and remove if it becomes 0
void decrementURI(List L, char str[]) {
    lockList(L);
    moveFront(L);
    char *currNode;
    while (lIndex(L) != -1) {
        currNode = L->cursor->uri;
        if (strcmp(str, currNode) == 0) {
            L->cursor->count -= 1;
            if (L->cursor->count == 0) {
                delete (L);
            }
            break;
        }
        moveNext(L);
    }
    unlockList(L);
}

// listReaderLock()
// reader locks cursors lock
void listReaderLock(List L, char uri[]) {
    lockList(L);
    rwlock_t *myLock;
    searchList(L, uri);
    myLock = L->cursor->rw;
    unlockList(L);
    reader_lock(myLock);
}

// listReaderUnlock()
// reader unlocks cursors lock
void listReaderUnlock(List L, char uri[]) {
    lockList(L);
    rwlock_t *myLock;
    searchList(L, uri);
    myLock = L->cursor->rw;
    unlockList(L);
    reader_unlock(myLock);
}

// listReaderLock()
// writer locks cursors lock
void listWriterLock(List L, char uri[]) {
    lockList(L);
    rwlock_t *myLock;
    searchList(L, uri);
    myLock = L->cursor->rw;
    unlockList(L);
    writer_lock(myLock);
}

// listReaderUnlock()
// writer unlocks cursors lock
void listWriterUnlock(List L, char uri[]) {
    lockList(L);
    rwlock_t *myLock;
    searchList(L, uri);
    myLock = L->cursor->rw;
    unlockList(L);
    writer_unlock(myLock);
}
