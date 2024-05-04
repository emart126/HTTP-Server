//--------------------------------
// Eric Martinez, CruzID:emart126
// List.h
//--------------------------------

#pragma once

#include <stdint.h>
#include <stdbool.h>

#define FORMAT "%d" // format string for List

// Exported types -------------------------------------------------------------
typedef int listElement;
typedef struct listObj *List;

// Constructors-Destructors ---------------------------------------------------

// newList()
// Creates and returns a new empty List.
List newList(void);

// freeList()
// Frees all heap memory associated with *pL, and sets
// *pL to NULL.
void freeList(List *pL);

// Access functions -----------------------------------------------------------

// length()
// Returns the number of elements in L.
// Pre: L!=NULL
int length(List L);

// index()
// Returns index of cursor element if defined, -1 otherwise.
int lIndex(List L);

// front()
// Returns front element of L
// Pre: length()>0
int front(List L);

// back()
// Returns back element of L
// Pre: length()>0
int back(List L);

// get()
// Returns cursor element of L
// Pre: length()>0, index()>=0
int get(List L);

// Manipulation procedures ----------------------------------------------------

// clear()
// Resets L to its original empty state.
// Pre: L is non empty
void clear(List L);

// set()
// Overwrites the cursor element’s data with x.
// Pre: length()>0, index()>=0
void set(List L, int x);

// moveFront()
// If L is non-empty, sets cursor under the front element, otherwise does nothing.
// Pre: L is not NULL
void moveFront(List L);

// moveBack()
// If L is non-empty, sets cursor under the back element, otherwise does nothing.
// Pre: L is not NULL
void moveBack(List L);

// movePrev()
// If cursor is defined and not at front, move cursor one
// step toward the front of L; if cursor is defined and at front,
// cursor becomes undefined; if cursor is undefined do nothing
// Pre: cursor is defined
void movePrev(List L);

// moveNext()
// If cursor is defined and not at back, move cursor one
// step toward the back of L; if cursor is defined and at back,
// cursor becomes undefined; if cursor is undefined do nothing
// Pre: cursor is defined
void moveNext(List L);

// append()
// Insert new element into L. If L is non-empty,
//insertion takes place after back element.
// Pre: L is not NULL
void append(List L, int x, char uri[]);

// deleteFront()
// Delete the front element.
// Pre: length()>0
void deleteFront(List L);

// deleteBack()
// Delete the back element.
// Pre: length()>0
void deleteBack(List L);

// delete()
// Delete cursor element, making cursor undefined.
// Pre: length()>0, index()>=0
void delete (List L);

// Other operations -----------------------------------------------------------

// searchList()
// searches list to see if it has a node with string str[]
// returns true if it is in list
int searchList(List L, char str[]);

// Lock List for use
void lockList(List L);

// Lock List for others to use
void unlockList(List L);

// incrementURI()
// appends new node with 1 and with uri
// if it exists, increments count by 1
void incrementURI(List L, char uri[]);

// decrementURI()
// decrements count in node, and removes it if it becomes 0
void decrementURI(List L, char uri[]);

// listReaderLock()
// reader locks cursors lock
void listReaderLock(List L, char uri[]);

// listReaderUnlock()
// reader unlocks cursors lock
void listReaderUnlock(List L, char uri[]);

// listReaderLock()
// writer locks cursors lock
void listWriterLock(List L, char uri[]);

// listReaderUnlock()
// writer unlocks cursors lock
void listWriterUnlock(List L, char uri[]);
