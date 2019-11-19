#include "FileSystem.h"
#include "func.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#define SizeFreeSpaceList 128
#define SizeInode 126
#define SizeKB 1024


typedef struct {
    uint8_t current_dir;
    char *current_diskname;
    uint8_t buffer[1024];
} Current_info;

Current_info *current_info;
Super_block *super_block;

//helper func
int *getChildIndex_handler(uint8_t index) {
    // -1 as null terminater
    int *indexes = (int*) malloc (sizeof(int)*SizeInode);
    int p = 0;
    for (int i=0; i<SizeInode; i++) {
        // should be in use
        if ((getSizeBit(super_block->inode[i].dir_parent) == index) & isHighestBitSet(super_block->inode[i].used_size)) {
            indexes[p] = i;
            p += 1;
        }
    }
    indexes[p] = -1;
    return indexes;
}

void delete_file_handler(int index) {
    int start = super_block->inode[index].start_block;
    int size = getSizeBit(super_block->inode[index].used_size);
    // printf("%d %d\n",start, start+size-1);
    printf("Deleting file %s\n", super_block->inode[index].name);
    setBitInRange(super_block->free_block_list, start, start+size-1, 0);
    memset(super_block->inode[index].name, 0, sizeof(super_block->inode[index].name));
    super_block->inode[index].used_size = 0;
    super_block->inode[index].start_block = 0;
    super_block->inode[index].dir_parent = 0;
}

void delete_empty_dir(int index) {
    printf("Deleting empty dir %s\n", super_block->inode[index].name);
    memset(super_block->inode[index].name, 0, sizeof(super_block->inode[index].name));
    super_block->inode[index].used_size = 0;
    super_block->inode[index].start_block = 0;
    super_block->inode[index].dir_parent = 0;
}

void delete_dir_handler(int index) {
    //TODO: need test
    if (!isHighestBitSet(super_block->inode[index].dir_parent)) {
        delete_file_handler(index);
        return;
    }
    int *indexes = getChildIndex_handler(index);
    if (indexes[0] == -1) {
        //empty dir
        delete_empty_dir(index);
        free(indexes);
        return;
    } 
    for (int i=0; indexes[i] != -1; i++) {
        delete_dir_handler(indexes[i]);
    }
    free(indexes);
}

void saveSuperBlock(void) {
    FILE *fp = fopen(current_info->current_diskname, "rb+");
    fwrite(&super_block->free_block_list, sizeof(super_block->free_block_list), 1, fp);
    fwrite(&super_block->inode, sizeof(super_block->inode), 1, fp);
    fclose(fp);
}

//helper function ends



void fs_mount(char *new_disk_name) {
    FILE *fp = fopen(new_disk_name, "rb");
    if (fp == NULL) {
        fprintf(stderr, "Error: Cannot find disk %s\n", new_disk_name);
        return;
    }
    Super_block *new_super_block = (Super_block*) malloc (sizeof(Super_block));
    fread(&new_super_block->free_block_list, sizeof(new_super_block->free_block_list), 1, fp);
    fread(&new_super_block->inode, sizeof(new_super_block->inode), 1, fp);
    fclose(fp);

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
    printf("freelist: %s\n", binary_free_list);
    printf("inode: %s\n", binary_str);
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
    current_info = temp_info;
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

    // printf("Inode index: %d\n", index);

    if (index == -1) {
        fprintf(stderr, "Error: Superblock in disk %s is full, cannot create %s\n", current_info->current_diskname, name);
        return;
    }

    // check name
    if ((strcmp(name, ".") == 0) || (strcmp(name,"..") == 0 || strlen(name) == 0 || strlen(name) > 5)) {
        fprintf(stderr, "Error: File or directory %s name is invalid\n",name);
        return;
    }

    int *indexes = getChildIndex_handler(current_info->current_dir);
    for (int i=0; indexes[i] != -1; i++) {
        // printf("filename: %d %s\n", i, super_block->inode[i].name);
        //ref: https://stackoverflow.com/questions/18437430/comparing-non-null-terminated-char-arrays
        if (strncmp(super_block->inode[i].name, name, 5) == 0){
            fprintf(stderr, "Error: File or directory %s already exists\n",name);
            free(indexes);
            return;
        }
    }
    free(indexes);
    
    // create dir
    if (size == 0) {
        printf("%s\n",stringToBinary(super_block->free_block_list));
        //name
        memcpy(super_block->inode[index].name, name, strlen(name));
        printf("name: %s\n", super_block->inode[index].name);
        //use size
        super_block->inode[index].used_size = setHighestBit(super_block->inode[index].used_size);
        printf("use size: %d\n", super_block->inode[index].used_size);
        //atart block
        super_block->inode[index].start_block = 0;
        printf("start block: %d\n", super_block->inode[index].start_block);
        // dir_parent
        super_block->inode[index].dir_parent = current_info->current_dir;
        super_block->inode[index].dir_parent = setHighestBit(super_block->inode[index].dir_parent);
        printf("dir_parent: %d\n", super_block->inode[index].dir_parent);

        saveSuperBlock();
        return;

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

    if (has_abaliable_block == 0) {
        fprintf(stderr, "Error: Cannot allocate %d on %s\n", size, current_info->current_diskname);
        return;
    }
    
    // set bits in free_block_list
    setBitInRange(super_block->free_block_list, start, end-1, 1);
    printf("%s\n",stringToBinary(super_block->free_block_list));
    // printf("sizeblock: %lu, available: %d, start: %d, end: %d, index: %d\n", strlen(binary_str), has_abaliable_block, start, end, index);
    free(binary_str);
    
    //name
    memcpy(super_block->inode[index].name, name, strlen(name));
    printf("name: %s\n", super_block->inode[index].name);
    //user size
    super_block->inode[index].used_size = size;
    super_block->inode[index].used_size = setHighestBit(super_block->inode[index].used_size);
    printf("use size: %d\n", super_block->inode[index].used_size);
    // start block
    super_block->inode[index].start_block = start;
    printf("start block: %d\n", super_block->inode[index].start_block);
    // dir_parent
    super_block->inode[index].dir_parent = current_info->current_dir;
    printf("dir_parent: %d\n", super_block->inode[index].dir_parent);

    // save
    saveSuperBlock();

    return;   
}


void fs_delete(char name[5]) {
    if (super_block == NULL) {
        fprintf(stderr,"Error: No file system is mounted\n");
        return;
    }

    int index = -1;
    for (int i=0; i<SizeInode; i++) {
        if (strncmp(super_block->inode[i].name, name, 5) == 0){
            index = i;
            break;
        }
    }
    if (index == -1) {
        fprintf(stderr, "Error: File or directory %s does not exist\n", name);
        return;
    }

    delete_dir_handler(index);
    // save
    saveSuperBlock();
}

void fs_read(char name[5], int block_num) {
    if (super_block == NULL) {
        fprintf(stderr,"Error: No file system is mounted\n");
        return;
    }

    int index = -1;
    for (int i=0; i<SizeInode; i++) {
        if ((strncmp(super_block->inode[i].name, name, 5) == 0) 
            && (getSizeBit(super_block->inode[i].dir_parent) == current_info->current_dir) 
            && (!isHighestBitSet(super_block->inode[i].dir_parent))){
            index = i;
            break;
        }
    }
    if (index == -1) {
        fprintf(stderr, "Error: File %s does not exist\n", name);
        return;
    }

    int size = getSizeBit(super_block->inode[index].used_size);
    if (block_num > size-1) {
        fprintf(stderr, "Error: %s does not have block %d\n", name, block_num);
        return;
    }
    int start = super_block->inode[index].start_block;
    int real_block = start + block_num;
    // printf("%d\n", real_block);
    FILE *fp = fopen(current_info->current_diskname, "rb");
    fseek(fp, real_block * SizeKB, SEEK_SET);
    fread(current_info->buffer, sizeof(current_info->buffer), 1, fp);
    // printf("%s\n", current_info->buffer);
    fclose(fp);
}

void fs_write(char name[5], int block_num) {
    if (super_block == NULL) {
        fprintf(stderr,"Error: No file system is mounted\n");
        return;
    }

    int index = -1;
    for (int i=0; i<SizeInode; i++) {
        if ((strncmp(super_block->inode[i].name, name, 5) == 0) 
            && (getSizeBit(super_block->inode[i].dir_parent) == current_info->current_dir) 
            && (!isHighestBitSet(super_block->inode[i].dir_parent))){
            index = i;
            break;
        }
    }
    if (index == -1) {
        fprintf(stderr, "Error: File %s does not exist\n", name);
        return;
    }
    int size = getSizeBit(super_block->inode[index].used_size);
    if (block_num > size-1) {
        fprintf(stderr, "Error: %s does not have block %d\n", name, block_num);
        return;
    }
    int start = super_block->inode[index].start_block;
    int real_block = start + block_num;
    // printf("real block: %d\n", real_block);
    // printf("%s\n", current_info->buffer);
    FILE *fp = fopen(current_info->current_diskname, "rb+");
    fseek(fp, real_block * SizeKB, SEEK_SET);
    fwrite(current_info->buffer, sizeof(current_info->buffer), 1, fp);
    fclose(fp);
}

void fs_buff(uint8_t buff[1024]) {
    memcpy(current_info->buffer, buff, 1024 * sizeof(uint8_t));
}