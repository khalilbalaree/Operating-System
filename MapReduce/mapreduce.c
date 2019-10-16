#include "mapreduce.h"
#include "threadpool.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#define word_size 20

typedef struct KeyValue_M{
    char *key;
    char *value;
    struct KeyValue_M *next;
} KeyValue_M;

typedef struct Key_M{
    char *key;
    struct Key_M *next;
} Key_M;

typedef struct {
    pthread_mutex_t lock;
    KeyValue_M *head;
    Key_M *front;
} Partition_M;

typedef struct {
    // pthread_mutex_t lock;
    Partition_M *partitions;
} Partition_Pool_M;

int n_rds;
Partition_Pool_M *pp;
Reducer reducer;

void free_pool_partition_malloc() {
    for (int i=0; i<n_rds;i++) {
        Key_M *temp_k;
        while (pp->partitions[i].front != NULL) {
            temp_k = pp->partitions[i].front;
            pp->partitions[i].front = pp->partitions[i].front->next;
            free(temp_k->key);
            free(temp_k);
        }
        pthread_mutex_lock(&(pp->partitions[i].lock));
        pthread_mutex_destroy(&(pp->partitions[i].lock));
    }
    free(pp->partitions);
    free(pp);
}

Partition_Pool_M *init_Partition() {
    Partition_Pool_M *partition_pool;
    partition_pool = (Partition_Pool_M *) malloc (sizeof(Partition_Pool_M));
    Partition_M *partitions = (Partition_M *) malloc (sizeof(Partition_M) * n_rds);
    for (int i=0;i<n_rds;i++) {
        partitions[i].head = NULL;
        partitions[i].front = NULL;
        pthread_mutex_init(&(partitions[i].lock), NULL);
    }

    partition_pool->partitions = partitions;

    return partition_pool;
}

void Insert_Partition(int partition_num, char *key, char *value) {
    // printf("%d: %s\n", partition_num,key);
    KeyValue_M *keyVal;  
    keyVal = (KeyValue_M *) malloc (sizeof(KeyValue_M)); 
    keyVal->key = (char*) malloc (sizeof(char)*word_size);
    keyVal->value =  (char*) malloc (sizeof(char));
    stpcpy(keyVal->key, key);
    strcpy(keyVal->value, value);
    keyVal->next = NULL;

    pthread_mutex_lock(&(pp->partitions[partition_num].lock));

    if (pp->partitions[partition_num].head == NULL) {
        // insert keyval
        pp->partitions[partition_num].head = keyVal;
        // insert keys
        Key_M *keys = (Key_M *) malloc (sizeof(Key_M));
        keys->key = (char*) malloc (sizeof(char)*word_size);
        strcpy(keys->key, key);
        keys->next = NULL;
        pp->partitions[partition_num].front = keys;
    } else {
        // insert keyval
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

        // insert keys
        Key_M *key_temp = pp->partitions[partition_num].front;
        while (key_temp->next != NULL && (strcmp(key_temp->key, key) != 0)) {
            key_temp = key_temp->next;
        }
        if (strcmp(key_temp->key, key) != 0) {
            Key_M *keys = (Key_M *) malloc (sizeof(Key_M));
            keys->key = (char*) malloc (sizeof(char)*word_size);
            strcpy(keys->key, key);
            keys->next = NULL;
            
            Key_M *key_temp = pp->partitions[partition_num].front;
            if (strcmp(key_temp->key, key) > 0) {
                keys->next = pp->partitions[partition_num].front;
                pp->partitions[partition_num].front = keys;
            } else {
                while ((key_temp->next != NULL) && (strcmp(key_temp->next->key, key) < 0)) {
                    key_temp = key_temp->next;
                }
                keys->next = key_temp->next;
                key_temp->next = keys;
                }
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
    int partition_num = MR_Partition(key, n_rds);
    // printf("%d\n", partition_num);
    Insert_Partition(partition_num, key, value);
}

void MR_ProcessPartition(int partition_number){
    Partition_M this_partition = pp->partitions[partition_number];
    Key_M *key_temp = this_partition.front;
    while (key_temp != NULL) {
        reducer(key_temp->key, partition_number);
        // printf("%d : %s\n",partition_number,key_temp->key);
        key_temp = key_temp->next;
    }
}

char *MR_GetNext(char *key, int partition_number){ 
    KeyValue_M *temp, *head;
    head = pp->partitions[partition_number].head;
    if (head == NULL || strcmp(head->key, key) != 0) {
        return NULL;
    } 

    pthread_mutex_lock(&(pp->partitions[partition_number].lock));
    pp->partitions[partition_number].head = head->next;
    temp = head;
    free(head);
    
    pthread_mutex_unlock(&(pp->partitions[partition_number].lock));
    
    return temp->value;
}

void *Thread_partition_run(void *arg) {
    int i = *((int *) arg);
    MR_ProcessPartition(i);
    free(arg);
    pthread_exit(NULL);
    return NULL;
}


void MR_Run(int num_files, char *filenames[], Mapper map, int num_mappers, Reducer concate, int num_reducers) {
    // master thread
    n_rds = num_reducers;
    pp = init_Partition();
    reducer = concate;

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