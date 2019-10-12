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
    KeyValue_M *head;
    int size;
    int number;
} Partition_M;

typedef struct {
    pthread_mutex_t lock;
    Partition_M *partitions;
} Partition_Pool_M;

int n_rds;
Partition_Pool_M *pp;

void free_pool_partition_malloc() {
    pthread_mutex_lock(&(pp->lock));
    pthread_mutex_destroy(&(pp->lock));
    free(pp->partitions);
    free(pp);
}

Partition_Pool_M *init_Partition() {
    Partition_Pool_M *partition_pool;
    partition_pool = (Partition_Pool_M *) malloc (sizeof(Partition_Pool_M));
    Partition_M *partitions = (Partition_M *) malloc (sizeof(Partition_M) * n_rds);
    for (int i=0;i<n_rds;i++) {
        partitions[i].number = i;
        partitions[i].size = 0;
        partitions[i].head = NULL;
    }

    pthread_mutex_init(&(partition_pool->lock), NULL);
    partition_pool->partitions = partitions;

    return partition_pool;
}

void Insert_Partition(int partition_num, char *key, char *value) {
    pthread_mutex_lock(&(pp->lock));
    KeyValue_M *keyVal;
    keyVal = (KeyValue_M *) malloc (sizeof(KeyValue_M));
    keyVal->key = key;
    keyVal->value = value;
    keyVal->next = NULL;

    if (pp->partitions[partition_num].head == NULL) {
        pp->partitions[partition_num].head = keyVal;
    } else {
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
    pp->partitions[partition_num].size++;

    pthread_mutex_unlock(&(pp->lock));
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
    int partition_num = MR_Partition(key, n_rds);
    // printf("%d\n", partition_num);
    Insert_Partition(partition_num, key, value);
}

void MR_ProcessPartition(int partition_number){

}

char *MR_GetNext(char *key, int partition_number){
    return NULL;
}


void MR_Run(int num_files, char *filenames[], Mapper map, int num_mappers, Reducer concate, int num_reducers) {
    // master thread
    n_rds = num_reducers;
    pp = init_Partition();

    ThreadPool_t *pool;
    pool = ThreadPool_create(num_mappers);
    if (pool == NULL) return;
    for (int i=0; i < num_files ;i++) {
        ThreadPool_add_work(pool, (void*)map, filenames[i]);
    }
    ThreadPool_destroy(pool);
    


    // KeyValue_M *keyval = pp->partitions[8].head;
    // while (keyval != NULL){
    //     printf("%s\n",keyval->key);
    //     keyval = keyval->next;
    // }


    free_pool_partition_malloc();  
}