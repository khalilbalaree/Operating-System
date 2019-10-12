#ifndef MAPREDUCE_H
#define MAPREDUCE_H
#include <pthread.h>

// function pointer types used by library functions
typedef void (*Mapper)(char *file_name);
typedef void (*Reducer)(char *key, int partition_number);

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


void MR_Run(int num_files, char *filenames[],
            Mapper map, int num_mappers,
            Reducer concate, int num_reducers);

void MR_Emit(char *key, char *value);

unsigned long MR_Partition(char *key, int num_partitions);

void MR_ProcessPartition(int partition_number);

char *MR_GetNext(char *key, int partition_number);
#endif