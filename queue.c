#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#include <pthread.h>
#include <assert.h>

typedef struct queue {
    int head;
    int tail;
    int len;
    int maxSize;
    void **buffer;
    pthread_mutex_t mutex;
    pthread_cond_t empty;
    pthread_cond_t full;
} queue_t;

// Dynamically allocates and initializes a new queue with a
// maximum size, size
// @param size the maximum size of the queue
// @return a pointer to a new queue_t
queue_t *queue_new(int size) {
    queue_t *q = (queue_t *) calloc(1, sizeof(queue_t));
    q->head = 0;
    q->tail = 0;
    q->len = 0;
    q->maxSize = size;
    q->buffer = (void **) calloc(size, sizeof(void *));

    int rc;
    rc = pthread_mutex_init(&(q->mutex), NULL);
    assert(!rc);
    rc = pthread_cond_init(&(q->empty), NULL);
    assert(!rc);
    rc = pthread_cond_init(&(q->full), NULL);
    assert(!rc);
    return q;
}

//  Delete your queue and free all of its memory.
//  @param q the queue to be deleted.  Note, you should assign the
//  passed in pointer to NULL when returning (i.e., you should set
//  *q = NULL after deallocation).
void queue_delete(queue_t **q) {
    if (*q != NULL) {
        pthread_mutex_destroy(&((*q)->mutex));
        pthread_cond_destroy(&((*q)->empty));
        pthread_cond_destroy(&((*q)->full));
        free((*q)->buffer);
        free(*q);
    }
    *q = NULL;
}

//  push an element onto a queue
//  @param q the queue to push an element into.
//  @param elem th element to add to the queue
//  @return A bool indicating success or failure.  Note, the function
//   should succeed unless the q parameter is NULL.
bool queue_push(queue_t *q, void *elem) {
    if (q == NULL) {
        return false;
    }

    pthread_mutex_lock(&(q->mutex));
    while (q->len == q->maxSize) {
        pthread_cond_wait(&(q->full), &(q->mutex));
    }

    q->buffer[q->tail] = elem;
    q->tail = (q->tail + 1) % q->maxSize;
    q->len = q->len + 1;

    pthread_cond_signal(&(q->empty));
    pthread_mutex_unlock(&(q->mutex));
    return true;
}

// @brief pop an element from a queue.
//  @param q the queue to pop an element from.
//  @param elem a place to assign the poped element.
//  @return A bool indicating success or failure.  Note, the function
//   should succeed unless the q parameter is NULL.
bool queue_pop(queue_t *q, void **elem) {
    if (q == NULL) {
        return false;
    }

    pthread_mutex_lock(&(q->mutex));
    while (q->len == 0) {
        pthread_cond_wait(&(q->empty), &(q->mutex));
    }

    *elem = q->buffer[q->head];
    q->head = (q->head + 1) % q->maxSize;
    q->len = q->len - 1;

    pthread_cond_signal(&(q->full));
    pthread_mutex_unlock(&(q->mutex));
    return true;
}
