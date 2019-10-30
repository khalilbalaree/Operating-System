# Cmput379 Assignment2

Name: Zijun Wu  
CCID: 1488834

# Data Structure
## 1. Mapreduce.c

### 1. Intermediate key/value pair:  

**struct KeyValue_M:**  
char key, value  
KeyValue_M *next

### 2. key/value pairs in each partition:
**struct Partition_M: Accending order single linked list contains KeyValue_M**  
Accending order single linked list with the pointer called head points to the head of the key/value pairs, the algorithm and a metux lock for each partition make sure after each insertion it is in the accending order.

### 3. Partition pool:
**struct Partition_Pool_M: Arrays of Partition_M**  
Arrays of Partition_M, and contains the number of reducer and the function of reducer

### 3. synchronization primitives:
Metux locks for each partition, lock when MR_Emit the specific partition

## 2. Threadpool
### 1. Scheduling:
Longest job first serve and put a job to an idle thread if the thread pool has one.
### 2. Implementation:
**Job queue: decending order single linked list contains ThreadPool_work_t**  
If the job queue is empty, put the job to the head of the queue and notify the worker thread to take it(dequeue) directly, the queue will degenerate to a single ThreadPool_work_t element.  
If the job queue is not empty, use stat syscall to calculate the size of the file, and insert in in the job queue.  

**From there is an idle queue -> no idle queue**  
If there is no idle thread, so they won't take the job from queue before finished current jobs, so add the jobs to work queue will make the queue no longer a single element(job-imediate).

### 3. synchronization primitives:
Metux lock for both add work and get work from ThreadPool_work_queue_t
A signal will be send when there is a new work added to the job queue and worker thread can get it immediately

# Time complexity
## 1. MR_Emit
MR_Emit() first call the function MR_Partition() to get the hash value according to key value, then put this key/value pair to the specific partition structure(Partition_M). Before insertion, the thread should get the metux lock for this partition, then insert the key value to the linked list, the algorithm makes sure after each insertion, the linked list is in accending order according to the key value. For the time complexity, because it is need to finded a right place to insert key, so for a single key, there's a loop to iterate through linked list which takes **O(n)**. Overall, the complexity is **O(n^2)**

## 2. MR_GetNext
MR_GetNext() takes a key value and a partition number as parameters, to make it efficient, this function does not need to get the metux lock because each thread just works on its own partition. Because of the accending order linked list, each time the reducer call MR_GetNext(), it gets the value of the head of the linked list and remove it from the linked list, it takes **O(1)**, so the complexity for excuting the whole data in one partition takes **O(n)**
 
# Testing
## 1. Thread pool
Printing when the thread pool add a job and get a job  
Example: For 2 thread and 4 jobs, output:   
add job    
get job   
add job  
get job  
add job   
add job  
get job  
get job 

## 2. Partitions:
Print the whole data inside one partition given some relatively small jobs
Example: raw data:  a and a and a and a and, output:
a->a->a->a->and->and->and->and  







