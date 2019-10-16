#include "threadpool.h"
#include "pthread.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

bool queue_isempty(ThreadPool_work_queue_t *queue){
    return (queue->head == NULL);
}

void free_pool_malloc(ThreadPool_t *tp) {
    pthread_mutex_lock(&(tp->lock));
    pthread_mutex_destroy(&(tp->lock));
    pthread_cond_destroy(&(tp->job_immediate));
    free(tp->threads);
    free(tp->queue);
    free(tp);
}

ThreadPool_work_t *ThreadPool_get_work(ThreadPool_t *tp){
    pthread_mutex_lock(&(tp->lock));

    // when tp->exit=1, job has finshed append
    while (queue_isempty(tp->queue) && !tp->exit) {
        pthread_cond_wait(&(tp->job_immediate), &(tp->lock));
    }

    // printf("get work\n");
    
    if (queue_isempty(tp->queue)) {
        pthread_mutex_unlock(&(tp->lock));
        return NULL;
    }

    // dequeue
    ThreadPool_work_t *work;
    work = tp->queue->head;
    tp->queue->head = tp->queue->head->next;

    pthread_mutex_unlock(&(tp->lock));

    return work;
}

bool ThreadPool_add_work(ThreadPool_t *tp, thread_func_t func, void *arg) {
    if (pthread_mutex_lock(&(tp->lock)) != 0) {
        return false;
    }
    // printf("add_work\n");

    ThreadPool_work_t *work;
    work = (ThreadPool_work_t *) malloc (sizeof(ThreadPool_work_t));
    work->func = func;
    work->arg = arg;
    work->next = NULL;
 
    // get file size
    struct stat st;
    if (stat(arg, &st) == 0) {
        work->file_size = st.st_size;  
    } else {
        work->file_size = 0;
    }
     
    if (!queue_isempty(tp->queue)) {
        // longest job first serve if no idle thread
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
        // there are idle thread(s)
        tp->queue->head = tp->queue->tail = work;
        pthread_cond_signal(&(tp->job_immediate));
    }

    pthread_mutex_unlock(&(tp->lock));
    
    // // sleep 1ns for workers to get metux
    nanosleep((const struct timespec[]){{0, 1L}}, NULL);

    return true;
}

void ThreadPool_destroy(ThreadPool_t *tp) {
    // printf("destory...\n");

    pthread_mutex_lock(&(tp->lock));
    tp->exit = 1;
    // wake up all worker threads
    pthread_cond_broadcast(&(tp->job_immediate));
    pthread_mutex_unlock(&(tp->lock));

    // wait all threads finish
    for (int i=0; i<tp->thread_size; i++) {
        pthread_join(tp->threads[i], NULL);
        // printf("%d Thread finished\n", i);
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
    pool->exit = 0;

    // check for metux and cond
    if (pthread_mutex_init(&(pool->lock), NULL) != 0 || pthread_cond_init(&(pool->job_immediate), NULL)) {
        free_pool_malloc(pool);
        return NULL;
    }
    
    // init work queue
    pool->queue->head = NULL;
    pool->queue->tail = NULL;

    for (int i=0; i<pool->thread_size; i++) {
        if (pthread_create(&(pool->threads[i]), NULL, (void *)Thread_run, pool) == 0) {
            // printf("%d thread created\n", i);      
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

        thread_func_t func = work->func;
        void *arg = work->arg;

        // execute
        (*func)(arg);
        free(work);

        // printf("finish\n");
    }

    pthread_exit(NULL);
    return NULL;
}
