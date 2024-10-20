//
// Created by 邹嘉旭 on 2024/1/15.
//

#ifndef LDACS_SIM_LD_PQUEUE_H
#define LDACS_SIM_LD_PQUEUE_H

#include "../global/ldacs_sim.h"

/**
  * returned error codes, everything except Q_OK should be < 0
  */
typedef enum queue_erros_e {
    Q_OK = 0,
    Q_ERR_INVALID = -1,
    Q_ERR_LOCK = -2,
    Q_ERR_MEM = -3,
    Q_ERR_NONEWDATA = -4,
    Q_ERR_INVALID_ELEMENT = -5,
    Q_ERR_INVALID_CB = -6,
    Q_ERR_NUM_ELEMENTS = -7
} queue_errors_t;

/** priority data type */
typedef unsigned long long pqueue_pri_t;

/** callback functions to get/set/compare the priority of an element */
typedef pqueue_pri_t (*pqueue_get_pri_f)(void *a);

typedef void (*pqueue_set_pri_f)(void *a, pqueue_pri_t pri);

typedef int (*pqueue_cmp_pri_f)(pqueue_pri_t next, pqueue_pri_t curr);


/** callback functions to get/set the position of an element */
typedef size_t (*pqueue_get_pos_f)(void *a);

typedef void (*pqueue_set_pos_f)(void *a, size_t pos);


/** debug callback function to print a entry */
typedef void (*pqueue_print_entry_f)(FILE *out, void *a);


/** the priority queue handle */
typedef struct pqueue_t {
    size_t size; /**< number of elements in this queue */
    size_t avail; /**< slots available in this queue */
    size_t step; /**< growth stepping setting */
    pqueue_cmp_pri_f cmppri; /**< callback to compare nodes */
    pqueue_get_pri_f getpri; /**< callback to get priority of a node */
    pqueue_set_pri_f setpri; /**< callback to set priority of a node */
    pqueue_get_pos_f getpos; /**< callback to get position of a node */
    pqueue_set_pos_f setpos; /**< callback to set position of a node */
    void **d; /**< The actualy queue in binary heap form */

    // multithread
    pthread_mutex_t *mutex;
    pthread_cond_t *cond_pop;
} pqueue_t;


/**
 * initialize the queue
 *
 * @param n the initial estimate of the number of queue items for which memory
 *     should be preallocated
 * @param cmppri The callback function to run to compare two elements
 *     This callback should return 0 for 'lower' and non-zero
 *     for 'higher', or vice versa if reverse priority is desired
 * @param setpri the callback function to run to assign a score to an element
 * @param getpri the callback function to run to set a score to an element
 * @param getpos the callback function to get the current element's position
 * @param setpos the callback function to set the current element's position
 *
 * @return the handle or NULL for insufficent memory
 */
pqueue_t *pqueue_init(size_t n,
                      pqueue_cmp_pri_f cmppri,
                      pqueue_get_pri_f getpri,
                      pqueue_set_pri_f setpri,
                      pqueue_get_pos_f getpos,
                      pqueue_set_pos_f setpos);


/**
 * free all memory used by the queue
 * @param q the queue
 */
void pqueue_free(pqueue_t *q);


/**
 * return the size of the queue.
 * @param q the queue
 */
size_t pqueue_size(const pqueue_t *q);


/**
 * insert an item into the queue.
 * @param q the queue
 * @param d the item
 * @return 0 on success
 */
int8_t pqueue_insert(pqueue_t *q, void *d);


/**
 * move an existing entry to a different priority
 * @param q the queue
 * @param new_pri the new priority
 * @param d the entry
 */
int8_t pqueue_change_priority(pqueue_t *q,
                              pqueue_pri_t new_pri,
                              void *d);


/**
 * pop the highest-ranking item from the queue.
 * @param q the queue
 * @return NULL on error, otherwise the entry
 */
int8_t pqueue_pop(pqueue_t *q, void **e);

int8_t pqueue_pop_wait(pqueue_t *q, void **e);


/**
 * remove an item from the queue.
 * @param q the queue
 * @param d the entry
 * @return 0 on success
 */
int8_t pqueue_remove(pqueue_t *q, void *d);

int8_t pqueue_flush(pqueue_t *pq, void (*free_func)(void *));


/**
 * access highest-ranking item without removing it.
 * @param q the queue
 * @return NULL on error, otherwise the entry
 */
int8_t pqueue_peek(pqueue_t *q, void **e);


/**
 * print the queue
 * @internal
 * DEBUG function only
 * @param q the queue
 * @param out the output handle
 * @param the callback function to print the entry
 */
void
pqueue_print(pqueue_t *q,
             FILE *out,
             pqueue_print_entry_f print);


/**
 * dump the queue and it's internal structure
 * @internal
 * debug function only
 * @param q the queue
 * @param out the output handle
 * @param the callback function to print the entry
 */
void
pqueue_dump(pqueue_t *q,
            FILE *out,
            pqueue_print_entry_f print);


/**
 * checks that the pq is in the right order, etc
 * @internal
 * debug function only
 * @param q the queue
 */
int pqueue_is_valid(pqueue_t *q);

bool pqueue_empty(const pqueue_t *q);

void *pqueue_wait_while(void *pqueue);


#endif //LDACS_SIM_LD_PQUEUE_H
