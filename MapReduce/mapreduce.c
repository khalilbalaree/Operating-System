#include "mapreduce.h"
#include "threadpool.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

typedef struct KeyValue_M{
    char *key;
    char *value;
    struct KeyValue_M *next;
} KeyValue_M;

typedef struct {
    pthread_mutex_t lock;
    KeyValue_M *head;
} Partition_M;

typedef struct {
    Partition_M *partitions;
    Reducer reducer;
    int num_reducers;
} Partition_Pool_M;

// global variable partition pool
Partition_Pool_M *pp;

void free_pool_partition_malloc() {
    for (int i=0; i<pp->num_reducers;i++) {
        pthread_mutex_lock(&(pp->partitions[i].lock));
        pthread_mutex_destroy(&(pp->partitions[i].lock));
    }
    free(pp->partitions);
    free(pp);
}

Partition_Pool_M *init_Partition(Reducer reducer, int num_reducers) {
    Partition_Pool_M *partition_pool;
    partition_pool = (Partition_Pool_M *) malloc (sizeof(Partition_Pool_M));
    partition_pool->reducer = reducer;
    partition_pool->num_reducers = num_reducers;

    Partition_M *partitions = (Partition_M *) malloc (sizeof(Partition_M) * num_reducers);
    for (int i=0;i<num_reducers;i++) {
        partitions[i].head = NULL;
        pthread_mutex_init(&(partitions[i].lock), NULL);
    }

    partition_pool->partitions = partitions;

    return partition_pool;
}

void Insert_Partition(int partition_num, char *key, char *value) {
    // printf("%d: %s\n", partition_num,key);
    KeyValue_M *keyVal;  
    keyVal = (KeyValue_M *) malloc (sizeof(KeyValue_M)); 
    keyVal->key = (char*) malloc (strlen(key)+1);
    keyVal->value =  (char*) malloc (strlen(value)+1);
    stpcpy(keyVal->key, key);
    strcpy(keyVal->value, value);
    keyVal->next = NULL;

    pthread_mutex_lock(&(pp->partitions[partition_num].lock));

    if (pp->partitions[partition_num].head == NULL) {
        // insert keyval
        pp->partitions[partition_num].head = keyVal;
    } else {
        // insert keyval in accending order by keys
        KeyValue_M *current;
        current = pp->partitions[partition_num].head;
        if (strcmp(current->key, key) > 0) {
            keyVal->next = pp->partitions[partition_num].head;
            pp->partitions[partition_num].head = keyVal;
        } else {
            while ((current->next != NULL) && (strcmp(current->next->key, key) < 0)) {
                current = current->next;
            }
            keyVal->next = current->next;
            current->next = keyVal;
        }
    }

    pthread_mutex_unlock(&(pp->partitions[partition_num].lock));
}

unsigned long MR_Partition(char *key, int num_partitions) {
    unsigned long hash = 5381; 
    int c; 
    while ((c = * key++) != '\0') {
        hash = hash * 33 + c; 
    }
    return hash % num_partitions;
}

void MR_Emit(char *key, char *value) {
    // printf("emit: %s\n", key);
    int partition_num = MR_Partition(key, pp->num_reducers);
    // printf("%d\n", partition_num);
    Insert_Partition(partition_num, key, value);
}

void MR_ProcessPartition(int partition_number){
    while (pp->partitions[partition_number].head != NULL) {
        char *key = (char*) malloc (strlen(pp->partitions[partition_number].head->key)+1);
        strcpy(key, pp->partitions[partition_number].head->key);
        pp->reducer(key, partition_number);
        free(key);
    }
}

char *MR_GetNext(char *key, int partition_number){ 
    KeyValue_M *head;
    char *value;
    head = pp->partitions[partition_number].head;
    if (head == NULL || strcmp(head->key, key) != 0) {
        return NULL;
    } 

    value = head->value;
    pp->partitions[partition_number].head = head->next;
    free(head->key);
    free(head->value);
    free(head);
     
    return value;
}

void *Thread_partition_run(void *arg) {
    int i = *((int *) arg);
    free(arg);
    MR_ProcessPartition(i);
    pthread_exit(NULL);
    return NULL;
}


void MR_Run(int num_files, char *filenames[], Mapper map, int num_mappers, Reducer concate, int num_reducers) {
    // master thread
    pp = init_Partition(concate, num_reducers);

    ThreadPool_t *pool_map;
    pool_map = ThreadPool_create(num_mappers);
    if (pool_map == NULL) {
        printf("Init mapper pool failed\n");
        return;
    }
    for (int i=0; i < num_files ;i++) {
        if (!ThreadPool_add_work(pool_map, (void*)map, filenames[i])) {
            i--;
        }
    }
    ThreadPool_destroy(pool_map);

    // create threads for reducer
    pthread_t *threads = (pthread_t *) malloc (sizeof(pthread_t) * num_reducers);
    for (int i=0; i<num_reducers; i++) {
        int *arg = malloc(sizeof(*arg));
        *arg = i;
        pthread_create(&(threads[i]), NULL, Thread_partition_run, arg);
    }

    for (int i=0; i<num_reducers; i++) {
        pthread_join(threads[i], NULL);
    }
    free(threads);
    
    free_pool_partition_malloc();  
}