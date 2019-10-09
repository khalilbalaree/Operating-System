#include "mapreduce.h"
#include "threadpool.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void MR_Emit(char *key, char *value) {

}

void MR_ProcessPartition(int partition_number){

}

char *MR_GetNext(char *key, int partition_number){
    return NULL;
}

unsigned long MR_Partition(char *key, int num_partitions) {
    unsigned long hash = 5381; 
    int c; 
    while ((c = * key++) != '\0') {
        hash = hash * 33 + c; 
    }
    return hash % num_partitions;
}

void MR_Run(int num_files, char *filenames[], Mapper map, int num_mappers, Reducer concate, int num_reducers) {
    // master thread
    // for (int i=0; i < num_files ;i++) {
    //     printf("%s\n",filenames[i]);
    // }
    ThreadPool_create(num_mappers);
    
}