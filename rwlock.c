#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <assert.h>

typedef struct rwlock {
    int priority;
    int N;
    int curr_N;
    int curr_wrs;
    int curr_rders;
    int wait_wrs;
    int wait_rders;
    pthread_cond_t reader;
    pthread_cond_t writer;
    pthread_mutex_t mutex;
} rwlock_t;

typedef enum { READERS, WRITERS, N_WAY } PRIORITY;

//  Dynamically allocates and initializes a new rwlock with
//  priority p, and, if using N_WAY priority, n.
//  @param The priority of the rwlock
//  @param The n value, if using N_WAY priority
//  @return a pointer to a new rwlock_t
rwlock_t *rwlock_new(PRIORITY p, uint32_t n) {
    rwlock_t *rw = (rwlock_t *) calloc(1, sizeof(rwlock_t));
    rw->priority = p;
    rw->N = n;
    rw->curr_N = 0;
    rw->curr_wrs = 0;
    rw->curr_rders = 0;
    rw->wait_wrs = 0;
    rw->wait_rders = 0;

    int rc;
    rc = pthread_mutex_init(&(rw->mutex), NULL);
    assert(!rc);
    rc = pthread_cond_init(&(rw->reader), NULL);
    assert(!rc);
    rc = pthread_cond_init(&(rw->writer), NULL);
    assert(!rc);
    return rw;
}

//  Delete your rwlock and free all of its memory.
//  @param rw the rwlock to be deleted.  Note, you should assign the
//  passed in pointer to NULL when returning (i.e., you should set *rw
//  = NULL after deallocation).
void rwlock_delete(rwlock_t **rw) {
    pthread_mutex_destroy(&((*rw)->mutex));
    pthread_cond_destroy(&(*rw)->reader);
    pthread_cond_destroy(&(*rw)->writer);
    free(*rw);
    *rw = NULL;
}

// acquire rw for reading
void reader_lock(rwlock_t *rw) {
    pthread_mutex_lock(&(rw->mutex));
    rw->wait_rders += 1;
    while ((rw->priority == WRITERS && rw->wait_wrs > 0)
           || (rw->priority == N_WAY && rw->curr_N >= rw->N && rw->wait_wrs > 0)
           || rw->curr_wrs > 0) {
        pthread_cond_wait(&(rw->reader), &(rw->mutex));
    }
    rw->curr_N += 1;
    rw->wait_rders -= 1;
    rw->curr_rders += 1;
    pthread_mutex_unlock(&(rw->mutex));
}

// release rw for reading--you can assume that the thread
// releasing the lock has *already* acquired it for reading.
void reader_unlock(rwlock_t *rw) {
    pthread_mutex_lock(&(rw->mutex));
    rw->curr_rders -= 1;
    if (rw->priority == N_WAY) {
        if (rw->curr_rders == 0) {
            pthread_cond_signal(&(rw->writer));
        }
    } else if (rw->priority == READERS) {
        if (rw->curr_rders == 0) {
            pthread_cond_signal(&(rw->writer));
        }
    } else if (rw->priority == WRITERS) {
        if (rw->wait_wrs > 0 && rw->curr_rders == 0) {
            pthread_cond_signal(&(rw->writer));
        }
    }
    pthread_mutex_unlock(&(rw->mutex));
}

// acquire rw for writing
void writer_lock(rwlock_t *rw) {
    pthread_mutex_lock(&(rw->mutex));
    rw->wait_wrs += 1;
    while (rw->curr_rders > 0 || rw->curr_wrs > 0
           || (rw->priority == N_WAY && rw->wait_rders > 0 && rw->curr_N == 0)) {
        pthread_cond_wait(&(rw->writer), &(rw->mutex));
    }
    rw->wait_wrs -= 1;
    rw->curr_wrs += 1;
    pthread_mutex_unlock(&(rw->mutex));
}

// release rw for writing--you can assume that the thread
// releasing the lock has *already* acquired it for writing.
void writer_unlock(rwlock_t *rw) {
    pthread_mutex_lock(&(rw->mutex));
    rw->curr_wrs -= 1;
    rw->curr_N = 0;
    if (rw->priority == N_WAY) {
        if (rw->wait_rders > 0) {
            for (int i = 0; i < rw->N; i += 1) {
                pthread_cond_signal(&(rw->reader));
            }
        } else {
            pthread_cond_signal(&(rw->writer));
        }
    } else if (rw->priority == READERS) {
        if (rw->wait_rders > 0) {
            pthread_cond_broadcast(&(rw->reader));
        } else {
            pthread_cond_signal(&(rw->writer));
        }
    } else if (rw->priority == WRITERS) {
        if (rw->wait_wrs > 0) {
            pthread_cond_signal(&(rw->writer));
        } else {
            pthread_cond_broadcast(&(rw->reader));
        }
    }
    pthread_mutex_unlock(&(rw->mutex));
}
