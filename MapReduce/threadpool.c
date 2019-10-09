#include "threadpool.h"
#include "pthread.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool queue_isempty(ThreadPool_work_queue_t *queue){
    return (queue->tail == NULL);
}

ThreadPool_work_t *ThreadPool_get_work(ThreadPool_t *tp){
    pthread_mutex_lock(&(tp->lock));

    if (queue_isempty(tp->queue)) {
        return NULL;
    }
    // dequeue
    ThreadPool_work_t *work;
    work = tp->queue->head;
    tp->queue->head = tp->queue->head->next;
    tp->queue->size--;

    pthread_mutex_unlock(&(tp->lock));

    return work;

}

bool ThreadPool_add_work(ThreadPool_t *tp, thread_func_t func, void *arg) {
    pthread_mutex_lock(&(tp->lock));
    ThreadPool_work_t *work;
    work = (ThreadPool_work_t *) malloc (sizeof(ThreadPool_work_t));
    work->func = func;
    work->arg = arg;
    work->next = NULL;

    if (!queue_isempty(tp->queue)) {
        tp->queue->tail->next = work;
        tp->queue->tail = work;
    } else {
        tp->queue->head = tp->queue->tail = work;
    }

    tp->queue->size++;

    pthread_mutex_unlock(&(tp->lock));

    return true;
}

void ThreadPool_destroy(ThreadPool_t *tp) {
    printf("destory...\n");
    tp->exit = 1;
    // wait all threads finish
    for (int i=0; i<tp->thread_size; i++) {
        pthread_join(tp->threads[i], NULL);
    }

    // free malloc
    if (tp->threads) {
        free(tp->threads);
    }
    if (tp->queue) {
        free(tp->queue);
    }
    free(tp);
}

ThreadPool_t *ThreadPool_create(int num) {
    ThreadPool_t *pool;
    pool = (ThreadPool_t *) malloc(sizeof(ThreadPool_t));

    // init threads
    pool->threads = (pthread_t *) malloc (sizeof(pthread_t) * num);
    pool->queue = (ThreadPool_work_queue_t *) malloc (sizeof(ThreadPool_work_queue_t));
    pool->thread_size = num;
    pool->exit = 0;
    
    // init work queue
    pool->queue->head = NULL;
    pool->queue->tail = NULL;
    pool->queue->size = 0;

    for (int i=0; i<pool->thread_size; i++) {
        if (pthread_create(&(pool->threads[i]), NULL, (void *)Thread_run, pool) == 0) {
            printf("%d thread created\n", i);        
        } else {
            ThreadPool_destroy(pool);
            return NULL;
        }
    }

    
    ThreadPool_destroy(pool);

    return pool;
}


void *Thread_run(ThreadPool_t *tp) {
    // printf("Hello!\n");
    // ThreadPool_t *pool = (ThreadPool_t *)threadpool;
    ThreadPool_work_t *work;

    for(;;) {
        work = ThreadPool_get_work(tp);
        if (work == NULL) {
            return NULL;
        } 
        
        thread_func_t func = work->func;
        void *arg = work->arg;

        // execute
        (*func)(arg);  


        if (tp->exit) {
            break;
        }
    }

    pthread_exit(NULL);
    return NULL;
}
