# CMPUT379 Assignment

Name: Zijun Wu  
CCID: 1488834


## Design details

### 1. General:

1. Support for triming whitespaces for user inputs
2. Support handling `ˆD` and empty input

### 2. Five levels' command analysis of user inputs

1. Backgrounding commands `&`
2. Multiple commands `;`
3. Redirecting commands `>`
4. Piping commands `|`
5. Single commands

### 2. Built-in commands:

1. cd: using system call `chdir()` in parent process  
2. pwd: using system call `getcwd()` in parent process  
3. $PATH: printing shell's variable `GLOBAL_PATH` in parent process, /bin/ and /usr/bin/ are initially included in the path
4. a2path: appending or overwriting string to variable `GLOBAL_PATH` in parent process
5. exit: using system call `_exit()`

### 3. External commands:
 
Using system call `execve()` in child process with the following control flow: file found -> execute file -> function return:

1. Find executable file from the command name directly.
2. Find executable file from the command name under the current directory  
3. Find executable file from the command name under each PATH in $PATH variable.

### 4. Backgrounding(&):

Create a new process for background command without calling wait in parent process. If there's a redirecting demand, redirect stdout to file, else redirect stdout to `"/dev/null"`

### 5. Multiple commands(;):

Split the command line with `;` and excute them one by one

### 6. Redirecting(>):

Redirect stdout of command to file using system call `dup2()`

### 7. Piping(|):

Create two processes for each command, and redirect the stdout of the first one to the stdin of the second one. Using `pipe()` for the interprocess communication.

### 8. Signal catching:

Supporting ctrl+C (SIGINT),ctrl+Z (SIGSTP),ctrl+X (SIGQUIT)


## Test case

1. `cd` -> dragonshell: expected argument to 'cd'  
`cd .. ; pwd` -> the upper path of the directory   
`cd ./Desktop/assgn1 ; pwd` -> /usr/Desktop/assgn1

2. `$PATH` -> /bin/:/usr/bin/  
`a2path` -> Dragonshell: a2path: No path detected  
`a2path $PATH:/usr/local/bin ; $PATH` -> Current PATH: /bin/:/usr/bin/:/usr/local/bin  
`a2path /usr/local/bin ; $PATH` -> Current PATH: /usr/local/bin  

3. `/usr/bin/touch` -> usage: touch [-A [-][[hh]mm]SS] [-acfhm] [-r file] [-t [[CC]YY]MMDDhhmm[.SS]] file ...  
`ls` -> Makefile       dragonshell     dragonshell.c  
`./dragonshell` -> Welcome to Dragon Shell! dragonshell >  

4. `ls > lsout.txt ; cat lsout.txt` -> Makefile       dragonshell     dragonshell.c  

5. `find ./ | sort` -> ./ .//Makefile .//README.md .//dragonshell .//dragonshell.c

6. `ls -al > lsout.txt ; find ./ | sort ; whereis gcc` -> ./ .//Makefile .//README.md . /dragonshell .//dragonshell.c .//lsout.txt /usr/bin/gcc

7. `find ./ | sort > find.txt; cat find.txt` -> ./ .//Makefile .//README.md .//dragonshell .//dragonshell.c

8. `ls &` -> PID 22812 is running in the background

9. `exit` -> back to normal shell  
`ˆC` -> Nothing happen  
`ˆZ` -> Nothing happen  
`ˆX` -> Nothing happen

## reference:

1. Use for trimming leading and trailing whitespace from a C string: https://stackoverflow.com/questions/122616/how-do-i-trim-leading-trailing-whitespace-in-a-standard-way

2. Use for catching signals:  signal1.c from eclass















