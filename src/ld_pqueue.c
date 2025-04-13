#include "ld_pqueue.h"


#define left(i)   ((i) << 1)
#define right(i)  (((i) << 1) + 1)
#define parent(i) ((i) >> 1)

static int8_t pqueue_lock_internal(const pqueue_t *q) {
    // all errors are unrecoverable for us
    if (0 != pthread_mutex_lock(q->mutex)) {
        return Q_ERR_LOCK;
    }
    return Q_OK;
}

static int8_t pqueue_unlock_internal(const pqueue_t *q) {
    // all errors are unrecoverable for us
    if (0 != pthread_mutex_unlock(q->mutex))
        return Q_ERR_LOCK;
    return Q_OK;
}


pqueue_t *pqueue_init(size_t n,
                      const pqueue_cmp_pri_f cmppri,
                      const pqueue_get_pri_f getpri,
                      const pqueue_set_pri_f setpri,
                      const pqueue_get_pos_f getpos,
                      const pqueue_set_pos_f setpos) {
    pqueue_t *q;

    if (!(q = malloc(sizeof(pqueue_t))))
        return NULL;

    q->mutex = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
    if (q->mutex == NULL) {
        free(q);
        return NULL;
    }
    pthread_mutex_init(q->mutex, NULL);

    q->cond_pop = (pthread_cond_t *) malloc(sizeof(pthread_cond_t));
    if (q->cond_pop == NULL) {
        pthread_mutex_destroy(q->mutex);
        free(q->mutex);
        free(q);
        return NULL;
    }
    pthread_cond_init(q->cond_pop, NULL);

    /* Need to allocate n+1 elements since element 0 isn't used. */
    if (!(q->d = malloc((n + 1) * sizeof(void *)))) {
        free(q);
        return NULL;
    }

    q->size = 1;
    q->avail = q->step = (n + 1); /* see comment above about n+1 */
    q->cmppri = cmppri;
    q->setpri = setpri;
    q->getpri = getpri;
    q->getpos = getpos;
    q->setpos = setpos;

    return q;
}


void pqueue_free(pqueue_t *q) {
    pthread_mutex_destroy(q->mutex);
    pthread_cond_destroy(q->cond_pop);
    free(q->mutex);
    free(q->cond_pop);
    free(q->d);
    free(q);
}


size_t pqueue_size(const pqueue_t *q) {
    /* queue element 0 exists but doesn't count since it isn't used. */
    return (q->size - 1);
}

bool pqueue_empty(const pqueue_t *q) {
    return !pqueue_size(q);
}

static void bubble_up(const pqueue_t *q, size_t i) {
    size_t parent_node;
    void *moving_node = q->d[i];
    pqueue_pri_t moving_pri = q->getpri(moving_node);

    for (parent_node = parent(i);
         ((i > 1) && q->cmppri(q->getpri(q->d[parent_node]), moving_pri));
         i = parent_node, parent_node = parent(i)) {
        q->d[i] = q->d[parent_node];
        q->setpos(q->d[i], i);
    }

    q->d[i] = moving_node;
    q->setpos(moving_node, i);
}


static size_t maxchild(pqueue_t *q, size_t i) {
    size_t child_node = left(i);

    if (child_node >= q->size)
        return 0;

    if ((child_node + 1) < q->size &&
        q->cmppri(q->getpri(q->d[child_node]), q->getpri(q->d[child_node + 1])))
        child_node++; /* use right child instead of left */

    return child_node;
}


static void percolate_down(pqueue_t *q, size_t i) {
    size_t child_node;
    void *moving_node = q->d[i];
    pqueue_pri_t moving_pri = q->getpri(moving_node);

    while ((child_node = maxchild(q, i)) &&
           q->cmppri(moving_pri, q->getpri(q->d[child_node]))) {
        q->d[i] = q->d[child_node];
        q->setpos(q->d[i], i);
        i = child_node;
    }

    q->d[i] = moving_node;
    q->setpos(moving_node, i);
}


int8_t pqueue_insert(pqueue_t *q, void *d) {
    void *tmp;
    size_t i;

    if (!q) return Q_ERR_INVALID;

    if (0 != pqueue_lock_internal(q))
        return Q_ERR_LOCK;

    /* allocate more memory if necessary */
    if (q->size >= q->avail) {
        size_t newsize = q->size + q->step;
        if (!((tmp = realloc(q->d, sizeof(void *) * newsize))))
            return 1;
        q->d = tmp;
        q->avail = newsize;
    }

    /* insert item */
    i = q->size++;
    q->d[i] = d;
    bubble_up(q, i);

    if (0 != pqueue_unlock_internal(q))
        return Q_ERR_LOCK;

    pthread_cond_signal(q->cond_pop);
    return 0;
}


int8_t pqueue_change_priority(pqueue_t *q,
                              pqueue_pri_t new_pri,
                              void *d) {
    size_t posn;
    pqueue_pri_t old_pri = q->getpri(d);

    if (0 != pqueue_lock_internal(q))
        return Q_ERR_LOCK;

    q->setpri(d, new_pri);
    posn = q->getpos(d);
    if (q->cmppri(old_pri, new_pri))
        bubble_up(q, posn);
    else
        percolate_down(q, posn);

    if (0 != pqueue_unlock_internal(q))
        return Q_ERR_LOCK;

    return Q_OK;
}


int8_t pqueue_remove(pqueue_t *q, void *d) {
    if (0 != pqueue_lock_internal(q))
        return Q_ERR_LOCK;

    size_t posn = q->getpos(d);
    q->d[posn] = q->d[--q->size];
    if (q->cmppri(q->getpri(d), q->getpri(q->d[posn])))
        bubble_up(q, posn);
    else
        percolate_down(q, posn);

    if (0 != pqueue_unlock_internal(q))
        return Q_ERR_LOCK;
    return Q_OK;
}

int8_t pqueue_flush(pqueue_t *pq, void (*free_func)(void *)) {
    size_t pq_sz = pqueue_size(pq);
    while (pq_sz--) {
        void *v;
        pqueue_pop(pq, &v);
        if (free_func) {
            free_func(v);
        }
    }

    return Q_OK;
}

static int8_t pqueue_pop_internal(pqueue_t *q, void **e, int (*action)(pthread_cond_t *, pthread_mutex_t *)) {
    if (!q) return Q_ERR_INVALID;
    if (0 != pqueue_lock_internal(q))
        return Q_ERR_LOCK;

    if (q->size == 1) {
        if (action == NULL) {
            *e = NULL;
            return Q_ERR_INVALID;
        } else {
            while (q->size == 1) {
                action(q->cond_pop, q->mutex);
            }
        }
    }

    *e = q->d[1];
    q->d[1] = q->d[--q->size];
    percolate_down(q, 1);

    if (0 != pqueue_unlock_internal(q))
        return Q_ERR_LOCK;

    return Q_OK;
}

int8_t pqueue_pop(pqueue_t *q, void **e) {
    return pqueue_pop_internal(q, e, NULL);
}

int8_t pqueue_pop_wait(pqueue_t *q, void **e) {
    return pqueue_pop_internal(q, e, pthread_cond_wait);
}

int8_t pqueue_peek(pqueue_t *q, void **e) {
    if (!q) return Q_ERR_INVALID;
    if (0 != pqueue_lock_internal(q))
        return Q_ERR_LOCK;
    //void *d;
    if (q->size == 1) {
        *e = NULL;
        return Q_OK;
    }
    *e = q->d[1];

    if (0 != pqueue_unlock_internal(q))
        return Q_ERR_LOCK;
    return Q_OK;
}


void pqueue_dump(pqueue_t *q,
                 FILE *out,
                 pqueue_print_entry_f print) {
    int i;

    if (0 != pqueue_lock_internal(q))
        return;
    fprintf(stdout, "posn\tleft\tright\tparent\tmaxchild\t...\n");
    for (i = 1; i < q->size; i++) {
        fprintf(stdout,
                "%d\t%d\t%d\t%d\t%ul\t",
                i,
                left(i), right(i), parent(i),
                (unsigned int) maxchild(q, i));
        print(out, q->d[i]);
    }

    if (0 != pqueue_unlock_internal(q))
        return;
}


static void set_pos(void *d, size_t val) {
    /* do nothing */
}


static void set_pri(void *d, pqueue_pri_t pri) {
    /* do nothing */
}


void pqueue_print(pqueue_t *q, FILE *out, pqueue_print_entry_f print) {
    pqueue_t *dup;
    void *e;
    if (0 != pqueue_lock_internal(q))
        return;

    dup = pqueue_init(q->size, q->cmppri, q->getpri, q->setpri, q->getpos, q->setpos);
    dup->size = q->size;
    dup->avail = q->avail;
    dup->step = q->step;

    memcpy(dup->d, q->d, (q->size * sizeof(void *)));

    pqueue_pop(dup, (void **) &e);
    while (e) {
        print(out, e);
        pqueue_pop(dup, (void **) &e);
    }

    pqueue_free(dup);
    if (0 != pqueue_unlock_internal(q))
        return;
}


static int subtree_is_valid(pqueue_t *q, int pos) {
    if (0 != pqueue_lock_internal(q))
        return Q_ERR_LOCK;
    if (left(pos) < q->size) {
        /* has a left child */
        if (q->cmppri(q->getpri(q->d[pos]), q->getpri(q->d[left(pos)])))
            return 0;
        if (!subtree_is_valid(q, left(pos)))
            return 0;
    }
    if (right(pos) < q->size) {
        /* has a right child */
        if (q->cmppri(q->getpri(q->d[pos]), q->getpri(q->d[right(pos)])))
            return 0;
        if (!subtree_is_valid(q, right(pos)))
            return 0;
    }

    if (0 != pqueue_unlock_internal(q))
        return Q_ERR_LOCK;
    return 1;
}

int pqueue_is_valid(pqueue_t *q) {
    return subtree_is_valid(q, 1);
}

void *pqueue_wait_while(void *pqueue) {
    void *node = NULL;
    while (pqueue_empty(pqueue)) {
        // if (node != NULL) break;
        usleep(50);
    }
    pqueue_pop(pqueue, &node);
    return node;
}
