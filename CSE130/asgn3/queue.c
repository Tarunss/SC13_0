#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#include <pthread.h>

// Code to implement Queue.h

// Structure of Queue. Definitions of variables should be in here.
typedef struct queue {
    // size of queue
    int csize;
    // length of full queue
    int length;
    // pointer to the array of pointers (our actual buffer)
    void **queue;
    // index for front and back of queue
    int in;
    int out;
    // pthread condition variables ()
    pthread_cond_t full;
    pthread_cond_t empty;
    // our mutex
    pthread_mutex_t mutex;
} queue_t;

//Function to initialize and create a new queue of size t. Should allocate memory, and mutex restrictions as well.
queue_t *queue_new(int size) {
    //allocate memory for queue_t pointer
    queue_t *newQ = (queue_t *) calloc(1, sizeof(queue_t));
    //allocate memory for **queue within queue_t
    newQ->queue = (void *) calloc(size, sizeof(void *));
    // set in and out respectively to zero
    newQ->in = 0;
    newQ->out = 0;
    // initialize length and current size
    newQ->csize = 0;
    newQ->length = size;
    //initialize mutex
    newQ->mutex = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;
    //initialize CVs
    newQ->empty = (pthread_cond_t) PTHREAD_COND_INITIALIZER;
    newQ->full = (pthread_cond_t) PTHREAD_COND_INITIALIZER;
    return newQ;
}

// Function to delete allocated memory from a queue. Should delete memory, and mutex restrictions as well.
void queue_delete(queue_t **q) {
    //destroy mutex lock, delete condition variables
    pthread_cond_destroy(&((*q)->full));
    pthread_cond_destroy(&((*q)->empty));
    pthread_mutex_destroy(&((*q)->mutex));
    //deallocate memory of buffer inside queue
    free((*q)->queue);
    //delete pointer to q and set it to NULL
    free(*q);
    *q = NULL;
}

//Push function of a queue. Should push an element into the back of a given queue.
bool queue_push(queue_t *q, void *elem) {
    //check to see if queue and elems aren't null
    if (q == NULL) {
        return false;
    }
    //acquire mutex lock
    pthread_mutex_lock(&q->mutex);
    //busy waiting if the queue is full
    while (q->csize == q->length) {
        pthread_cond_wait((&q->full), (&q->mutex));
    }
    //out of busy waiting, set queue[in] to elem
    q->queue[q->in] = elem;
    //set in to right index
    q->in = (q->in + 1) % q->length;
    //increment csize
    q->csize++;
    //printf("successfull push\n");
    //unlock
    pthread_mutex_unlock(&q->mutex);
    //signal for empty (wake up all threads that are waiting on an empty queue)
    pthread_cond_broadcast(&q->empty);
    return true;
}

//Pop function. Should pop an element out from the front of a given queue.
bool queue_pop(queue_t *q, void **elem) {
    //check to see if pointers are valid
    if (q == NULL) {
        return false;
    }
    //acquire mutex lock
    pthread_mutex_lock(&q->mutex);
    //busy wait (on an empty queue instead)
    while (q->csize == 0) {
        pthread_cond_wait((&q->empty), (&q->mutex));
    }
    //out of busy waiting, do pop functionality
    *elem = q->queue[q->out];
    q->out = (q->out + 1) % q->length;
    //decrement csize
    q->csize--;
    //printf("successfull pop\n");

    //unlock
    pthread_mutex_unlock(&q->mutex);
    //signal for full (wake up all threads that are waiting on a full queue)
    pthread_cond_broadcast(&q->full);
    return true;
}
