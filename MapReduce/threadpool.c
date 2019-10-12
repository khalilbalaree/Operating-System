#include "threadpool.h"
#include "pthread.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

bool queue_isempty(ThreadPool_work_queue_t *queue){
    return (queue->head == NULL);
}

void free_pool_malloc(ThreadPool_t *tp) {
    pthread_mutex_lock(&(tp->lock));
    pthread_mutex_destroy(&(tp->lock));
    pthread_cond_destroy(&(tp->newjob));
    free(tp->threads);
    free(tp->queue);
    free(tp);
}

ThreadPool_work_t *ThreadPool_get_work(ThreadPool_t *tp){
    pthread_mutex_lock(&(tp->lock));
    
    // // // when tp->exit=1, job has finshed append
    // while (!tp->hasNewJob && !tp->exit) {
    //     pthread_cond_wait(&(tp->newjob), &(tp->lock));
    // }
    // tp->hasNewJob = 0;
    
    if (queue_isempty(tp->queue)) {
        pthread_mutex_unlock(&(tp->lock));
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

    // get file size
    struct stat st;
    stat(arg, &st);
    work->file_size = st.st_size;

    // printf("filesize: %d\n", work->file_size);

    if (!queue_isempty(tp->queue)) {
        ThreadPool_work_t *current;
        current = tp->queue->head;
        int max_size = 0;
        while (current != NULL) {
            if (current->file_size > max_size) {
                max_size = current->file_size;
            }
            current = current->next;
        }
        if (work->file_size > max_size) {
            work->next = tp->queue->head;
            tp->queue->head = work;
        } else {
            tp->queue->tail->next = work;
            tp->queue->tail = work;
        }
    } else {
        tp->queue->head = tp->queue->tail = work;
    }

    tp->queue->size++;
     
    // notify the worker can grab a job
    // tp->hasNewJob = 1;
    // pthread_cond_signal(&(tp->newjob));
    pthread_mutex_unlock(&(tp->lock));
    
    return true;
}

void ThreadPool_destroy(ThreadPool_t *tp) {
    printf("destory...\n");

    // pthread_cond_broadcast(&(tp->newjob));

    tp->exit = 1;

    // wait all threads finish
    for (int i=0; i<tp->thread_size; i++) {
        pthread_join(tp->threads[i], NULL);
    }

    // free malloc
    free_pool_malloc(tp);
}

ThreadPool_t *ThreadPool_create(int num) {
    ThreadPool_t *pool;
    pool = (ThreadPool_t *) malloc(sizeof(ThreadPool_t));

    // init threads
    pool->threads = (pthread_t *) malloc (sizeof(pthread_t) * num);
    pool->queue = (ThreadPool_work_queue_t *) malloc (sizeof(ThreadPool_work_queue_t));
    pool->thread_size = num;
    pool->exit = pool->running = pool->hasNewJob = 0;

    // check for metux and cond
    if (pthread_mutex_init(&(pool->lock), NULL) != 0 || pthread_cond_init(&(pool->newjob), NULL)) {
        free_pool_malloc(pool);
        return NULL;
    }
    
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

    return pool;
}


void *Thread_run(ThreadPool_t *tp) {
    // printf("Hello!\n");
    // ThreadPool_t *pool = (ThreadPool_t *)threadpool;
    ThreadPool_work_t *work;

    for(;;) {
        work = ThreadPool_get_work(tp);
        if (work == NULL && tp->exit) {
            break;
        } else if (work == NULL) {
            continue;
        }

        // printf("new job!\n");
        thread_func_t func = work->func;
        void *arg = work->arg;
        
        // pthread_mutex_lock(&(tp->lock));
        tp->running++;
        printf("start %d\n", tp->running);
        // pthread_mutex_unlock(&(tp->lock));

        // execute
        (*func)(arg); 

        // pthread_mutex_lock(&(tp->lock));
        tp->running--;
        printf("finish %d\n", tp->running);
        // pthread_mutex_unlock(&(tp->lock));
        
    }

    pthread_exit(NULL);
    return NULL;
}
