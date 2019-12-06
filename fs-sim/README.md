# 379 Assignment 3
### Name: Zijun Wu
### Student id: 1488834

# Design choice
## General helper functions
### stringToBinary
Visualize the free block list to 128 binary string
### isHighestBitSet && setHighestBit
Compare if the 7th bit in uint8_t is set to 1; or set it
### getSizeBit
get value of lowest 6th bits in uint8_t
### setBitInRange
set or unset binary directly in free block list
## fs_mount
Init a new temp super block called new_super_block, if it can pass 6 consistancy checks,then assign it to global super block instance. And set current dir to 127 which is root. If the mount is not successful, the global super block is NULL or the previous one, at each following commands first check the super block is NULL or not.
## fs_create
First scan if there is free inode, then get children of the current dir to check if the new file or dir is unique in current dir. If it is a new dir, then assign it to indoe directly. If it is a file, scan free block list use stringToBinary to scan to get the start position in data block, then assign it to a inode and set the corresponding bits in free block list.
## fs_delete
Use Two helper functions to delete dirs and files. After recursicely called, zero out the inode and if it is a file, zero out the data block and the corresponding bits in free block list.
## fs_read && fs_write
Check the name is existed and it is a file, then check block num is in range of file size. Use fseek() to move the file pointer to the read block num which is 1024*(start+block_num). Then read or write 1KB buffer to the file.
## fs_buff
Use memset() to flush the buffer, then for looping to write the buffer byte by byte.
## fs_ls
Check if the current dir is root, if yes, treat '.' and '..' as the same. Also since the name can have no null terminator, so use printf("-5.5s", name) to prevent unexpected output.
## fs_resize
Find the inode with the name, if the new size is less than old one, then zero out the extra data block and change the corresponding free block list. If it is larger, then check if there's enough space to allocate, else, find a new start position as fs_create(), copy the whole data to the new place and zero the origin.
## fs_defrag
Using two integer as pointer one is to loop the free block list to find the start of each inode, and the other is to keep track of the lowest position which can connect the end and the start of two inodes. While defraging, use a local buffer to hold the data, then zero the origin blocks, finally copy to the new blocks.
## fs_cd
Find the inode with the name if it is a dir, and set the current dir variable to its index. Or '..', find the parent index according to current dir's parent.
## System call
### No system call used, use C library funstions in the program
read->fread, open->fopen...

# Testing
## For checking the mounting
Using the consistency-check posted on eclass, print the stderr as required.
## For checking other opeartions
Using 4 test cases, check if the stderr and stdout are the same. And use cmp -b to compare if the disk is the same as the result disk in test case. And the program performed as required.