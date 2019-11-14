#include "FileSystem.h"
#include "func.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#define SizeFreeSpaceList 128
#define SizeInode 126


typedef struct {
    uint8_t current_dir;
    char *current_diskname;
    FILE *fp;
} Current_info;

Current_info *current_info;
Super_block *super_block;

void fs_mount(char *new_disk_name) {
    FILE *fp = fopen(new_disk_name, "rb");
    if (fp == NULL) {
        fprintf(stderr, "Error: Cannot find disk %s\n", new_disk_name);
        return;
    }
    Super_block *new_super_block = (Super_block*) malloc (sizeof(Super_block));
    fread(&new_super_block->free_block_list, sizeof(new_super_block->free_block_list), 1, fp);
    fread(&new_super_block->inode, sizeof(new_super_block->inode), 1, fp);

    // check consistancy 1  
    char *binary_str = (char*) malloc (SizeFreeSpaceList+1); 
    for (int i=0; i<SizeFreeSpaceList; i++) {
        if (i==0) {
            strcat(binary_str, "1");
        } else {
            strcat(binary_str, "0");
        }
    }
    for (int i=0; i<SizeInode; i++) {
        if (isHighestBitSet(new_super_block->inode[i].used_size)) {
            int node_size = getSizeBit(new_super_block->inode[i].used_size);
            int start_index = new_super_block->inode[i].start_block;
            for (int j=start_index; j<start_index+node_size; j++) {
                if (binary_str[j] == '1') {
                    free(binary_str);
                    free(new_super_block);
                    fprintf(stderr,"Error: File system in %s is inconsistent (error code: 1)\n", new_disk_name);
                    return;
                } else {
                    binary_str[j] = '1';
                }
            }
        }
    }
    char *binary_free_list = stringToBinary(new_super_block->free_block_list);
    if (strcmp(binary_free_list, binary_str) != 0) {
        fprintf(stderr,"Error: File system in %s is inconsistent (error code: 1)\n", new_disk_name);
        free(binary_str);
        free(binary_free_list);
        free(new_super_block);
        return;
    }

    // check consistancy 2
    for (int i=0; i<SizeInode; i++) {
        for (int j=i+1; j<SizeInode; j++) {
            if (isHighestBitSet(new_super_block->inode[i].used_size) && isHighestBitSet(new_super_block->inode[j].used_size)
                && (strcmp(new_super_block->inode[i].name, new_super_block->inode[j].name) == 0) 
                && (getSizeBit(new_super_block->inode[i].dir_parent) == getSizeBit(new_super_block->inode[j].dir_parent))) {
                    fprintf(stderr,"Error: File system in %s is inconsistent (error code: 2)\n", new_disk_name);
                    free(new_super_block);
                    return;
                } 
        }
    }

    // check consistancy 3
    for (int i=0; i<SizeInode; i++) {
        if (!isHighestBitSet(new_super_block->inode[i].used_size) 
            && new_super_block->inode[i].dir_parent == 0
            && new_super_block->inode[i].start_block == 0
            && new_super_block->inode[i].used_size == 0) {
                continue;
        } else if (isHighestBitSet(new_super_block->inode[i].used_size)
            && (strlen(new_super_block->inode[i].name) != 0)){
                // TODO: check bits
                continue;
        } else {
            fprintf(stderr,"Error: File system in %s is inconsistent (error code: 3)\n", new_disk_name);
            free(new_super_block);
            return;
        }     
    }

    // check consistancy 4
    for (int i=0; i<SizeInode; i++) {
        if (!isHighestBitSet(new_super_block->inode[i].dir_parent) && isHighestBitSet(new_super_block->inode[i].used_size)
            && (new_super_block->inode[i].start_block < 1 || new_super_block->inode[i].start_block > 127)){
            fprintf(stderr,"Error: File system in %s is inconsistent (error code: 4)\n", new_disk_name);
            free(new_super_block);
            return;
        }
    }

    // check consistancy 5
    for (int i=0; i<SizeInode; i++) {
        if (!isHighestBitSet(new_super_block->inode[i].used_size)){
            if ((getSizeBit(new_super_block->inode[i].used_size) == 0)
                && (new_super_block->inode[i].start_block == 0)) {
                    continue;
            } else if ((getSizeBit(new_super_block->inode[i].used_size) != 0)
                && (new_super_block->inode[i].start_block != 0)) {
                    continue;
            } else {
                fprintf(stderr,"Error: File system in %s is inconsistent (error code: 5)\n", new_disk_name);
                free(new_super_block);
                return;
            }
        }
    }

    // check consistancy 6
    for (int i=0; i<SizeInode; i++) {
        if (getSizeBit(new_super_block->inode[i].dir_parent) == 126) {
            fprintf(stderr,"Error: File system in %s is inconsistent (error code: 6)\n", new_disk_name);
            free(new_super_block);
            return;
        } else if (isHighestBitSet(new_super_block->inode[i].used_size)
            && (getSizeBit(new_super_block->inode[i].dir_parent) >= 0 && getSizeBit(new_super_block->inode[i].dir_parent) <= 125)) {
                if (!isHighestBitSet(new_super_block->inode[getSizeBit(new_super_block->inode[i].dir_parent)].used_size)
                    || !isHighestBitSet(new_super_block->inode[getSizeBit(new_super_block->inode[i].dir_parent)].dir_parent)) {
                        fprintf(stderr,"Error: File system in %s is inconsistent (error code: 6)\n", new_disk_name);
                        free(new_super_block);
                        return;
                    }
        }
    }

    // init global var
    super_block = new_super_block;
    Current_info *temp_info = (Current_info*) malloc (sizeof(Current_info));
    temp_info->current_dir = 127;
    char *current_diskname = malloc (sizeof(new_disk_name)+1);
    strcpy(current_diskname, new_disk_name);
    temp_info->current_diskname = current_diskname;
    temp_info->fp = fp;
    current_info = temp_info;
}

int *getChildIndex(uint8_t index) {
    int *indexes = (int*) malloc (sizeof(int)*SizeInode);
    int start = 0;
    for (int i=0; i<SizeInode; i++) {
        if (getSizeBit(super_block->inode[i].dir_parent) == index) {
            indexes[start] = i;
        } else {
            indexes[start] = -1;
        }
        start += 1;
    }
    return indexes;
}

void fs_create(char name[5], int size) {
    if (super_block == NULL) {
        fprintf(stderr,"Error: No file system is mounted\n");
        return;
    }

    int index = -1;
    // check available Inode
    for (int i=0;i<SizeInode;i++) {
        if (!isHighestBitSet(super_block->inode[i].used_size)){
            index = i;
            break;
        }
    }
    if (index == -1) {
        fprintf(stderr, "Error: Superblock in disk %s is full, cannot create %s\n", current_info->current_diskname, name);
        return;
    }
    // printf("index: %d\n", index);

    // check name
    int *indexes = getChildIndex(current_info->current_dir);
    for (int i=0; indexes[i]!=-1; i++) {  
        if (strcmp(super_block->inode[indexes[i]].name, name) == 0){
            fprintf(stderr, "Error: File or directory %s already exists\n",name);
            return;
        }
    }

    // check available block
    int start = 0; // inclusive
    int end = start + size; // exclusive
    int has_abaliable_block = 0;
    char *binary_str = stringToBinary(super_block->free_block_list);
    while (end <= SizeFreeSpaceList){ 
        int j = start;
        while (j < end){
            if (binary_str[j] != '0') {
                break;
            }
            j += 1;
        }
        if (j == end) {
            has_abaliable_block = 1;
            break;
        } else {
            start += 1;
            end += 1;
        }     
    }
    
    // set bits in free_block_list
    setBitInRange(super_block->free_block_list, start, end-1);
    printf("%s\n",stringToBinary(super_block->free_block_list));
    printf("sizeblock: %lu, available: %d, start: %d, end: %d\n", strlen(binary_str), has_abaliable_block, start, end);
    free(binary_str);
    
    if (has_abaliable_block == 0) {
        fprintf(stderr, "Error: Cannot allocate %d on %s\n", size, current_info->current_diskname);
        return;
    }


    
    
    
}
