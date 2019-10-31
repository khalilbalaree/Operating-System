# Cmput379 Assignment2

Name: Zijun Wu  
CCID: 1488834

# Data Structure
## 1. Mapreduce

### 1. Intermediate key/value pair:  

**struct KeyValue_M:**  
char key, value  
KeyValue_M *next

### 2. key/value pairs in each partition:
**struct Partition_M: Accending order single linked list contains KeyValue_M**  
Accending order single linked list with the pointer called head points to the head of the key/value pairs, the algorithm and a metux lock for each partition make sure after each insertion it is in the accending order.

### 3. Partition pool(Global Variable):
**struct Partition_Pool_M: Arrays of Partition_M**  
Arrays of Partition_M, and contains the number of reducer and the function of reducer

### 4. Synchronization primitives:
Metux locks for each partition, lock the specific partition when MR_Emit() is called

## 2. Thread pool
### 1. Scheduling:
Longest job first serve and put a job to an idle thread if the thread pool has one.
### 2. Implementation:
**Job queue: decending order single linked list contains ThreadPool_work_t**  
1. If the job queue is empty, put the job to the head of the queue and notify the worker thread to take it(dequeue) directly, in this case the queue will degenerate to a single ThreadPool_work_t element.  
2. If the job queue is not empty, use stat syscall to calculate the size of the file, and insert in in the job queue.  

**From there is an idle queue -> no idle queue**  
If there is no idle thread, so they won't take the job from queue before finished current jobs, so add the jobs to work queue will make the queue no longer a single element(job-imediate).

### 3. synchronization primitives:
1. Metux lock for both add work to and get work from ThreadPool_work_queue_t
2. A signal will be send when there is a new work added to the empty job queue and worker thread can get it immediately

# Handler functions
## 1. Insert_Partition(int partition_num, char *key, char *value)
Called by MR_Emit(), insert the key/value pair into the particular partition correctly.

## 2. Partition_Pool_M *init_Partition(Reducer reducer, int num_reducers)
Called by MR_Run(), initialize the global variable Partition_Pool *pp before running mappers

## 3. void free_pool_partition_malloc() & void free_pool_malloc(ThreadPool_t *tp)
Free malloc for both partition pool and thread pool

# Time complexity
## 1. MR_Emit
MR_Emit() first call the function MR_Partition() to get the hash value according to key value, then put this key/value pair to the specific partition structure(Partition_M). Before insertion, the thread should get the metux lock for this partition, then insert the key value to the linked list, the algorithm makes sure after each insertion, the linked list is in accending order according to the key value. For the time complexity, because it is need to finded a right place to insert key, so for a single key, there's a loop to iterate through linked list which takes **O(n)**

## 2. MR_GetNext
MR_GetNext() takes a key value and a partition number as parameters, to make it efficient, this function does not need to get the metux lock because each thread just works on its own partition. Because of the accending order linked list, each time the reducer call MR_GetNext(), it gets the value of the head of the linked list and remove it from the linked list, which takes **O(1)**
 
# Testing
## 1. Thread pool
Printing when the thread pool add a job, get a job and finshing adding 
Example: For 2 threads and 4 jobs, output is:   
add job    
get job   
add job  
get job  
add job   
add job  
finish adding  
get job  
get job 

**Correctness:** there are 2 idle queue initially, so they get the job directly after adding, when there are no idle queue, they are inserted into a job queue by longest-job-first-serve rule

## 2. Partitions:
Print the accending order linked list inside one partition given some relatively small jobs  
Example raw data: a and a and a and a and, output is:  
a->a->a->a->and->and->and->and  

**Correctness:** by this simple data which can hash to the same partition, the keys are in accending order in the linked list.

# Reference
1. Hash function provided in assignment pdf
2. Accending order linked list in #c: (https://stackoverflow.com/questions/21788598/c-inserting-into-linked-list-in-ascending-order)








