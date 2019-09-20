#include "dragonshell.h"
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#define TEXTSIZE 1000
#define LISTEXTSIZE 100

/**
 * @brief Tokenize a C string 
 * @param str - The C string to tokenize 
 * @param delim - The C string containing delimiter character(s) 
 * @param argv - A char* array that will contain the tokenized strings
 * Make sure that you allocate enough space for the array.
 */
void tokenize(char* str, const char* delim, char ** argv) {
  char* token;
  token = strtok(str, delim);
  for(size_t i = 0; token != NULL; ++i){
    argv[i] = token;
    token = strtok(NULL, delim);
  }
}


void scan_line(char *str) {
	printf("dragonshell > ");
  if (fgets(str, TEXTSIZE, stdin)) {
    size_t size = strlen(str);
    if (size > 1) {
		  str[size-1] = '\0';
	  }
  }
}

char *pwd_command() { 
  char *buffer = (char*) calloc(TEXTSIZE, sizeof(char));
  if (getcwd(buffer, TEXTSIZE) == NULL) {
    printf("pwd command failed\n");
  }
  // free(buffer);
  return buffer;
}

void cd_command(char *dir) {
  if (dir == NULL) {
    printf("dragonshell: expected argument to 'cd'\n");
    return;
  } 
  if (chdir(dir) != 0) {
    printf("dragonshell: No such file or directory\n");
  }
}

void init_globalpath(char *GLOBAL_PATH) {
  strcat(GLOBAL_PATH, "/bin/:/usr/bin/");
}

// TODO : PATH var pass back!!!!!!!!!!!!!!!!!!!!!!!!!
void a2path(char *path, char *GLOBAL_PATH) {
  char *env, **args, *newpath;
  int start;
  args = (char**) calloc(LISTEXTSIZE, sizeof(char*));
  newpath = (char*) calloc(TEXTSIZE, sizeof(char));
  tokenize(path, ":", args);
  if (strcmp(args[0], "$PATH") == 0) {
    // env = getenv("PATH");
    // if (env == NULL) {
    //   printf("No $PATH found\n");
    //   return;
    // }
    // strcat(newpath, env);
    strcat(newpath, GLOBAL_PATH);
    if (args[1]) {
      strcat(newpath, ":");
    }
    start = 1;
  }
  
  for (size_t i=start; args[i]; ++i){
    // printf("%s\n", args[i]);
    strcat(newpath, args[i]);  
    if (args[i+1] != NULL) {
      strcat(newpath, ":");
    }
  }
  // setenv("PATH", newpath, 1); 
  strcpy(GLOBAL_PATH, newpath);
  printf("%s\n", GLOBAL_PATH);
  free(args);
  free(newpath);
}


// bool excute_external_looptest(char * shellpath, char **args) { 
//   int status, ret;
//   pid_t pid = fork();

//   char *path = (char*) calloc(TEXTSIZE, sizeof(char));
//   strcat(path, shellpath);
//   strcat(path, "/");
//   strcat(path, args[0]);

//   if (pid == 0) {
//     // char *argv[] = {"bash", "-c", line,  NULL};
//     // execve("/bin/bash", argv, NULL);
//     // char *argv[] = {"touch", "readme.md", line,  NULL};
//     ret = execve(path, args, NULL);
//   } else if (pid > 0) {
//     if((pid = wait(&status)) < 0){
//       perror("wait");
//       _exit(1);
//     }
//     // if (WIFEXITED(status)){
//     //   ret = WEXITSTATUS(status);
//     // }
//   } else {
//     perror("fork failed");
//   }
//   printf("%s %d\n",shellpath, ret);
//   free(path);
//   return ret != -1; 
// }

bool excute_external_looptest(char **tests, char **args) { 
  int status, ret;
  


  for (size_t i=0; tests[i]; ++i){
    ret = 0;
    char *path = (char*) calloc(TEXTSIZE, sizeof(char));
    pid_t pid = fork();
    printf("child process in loop: [%d]\n", pid);
    if (pid == 0) {
        strcat(path, tests[i]);
        strcat(path, "/");
        strcat(path, args[0]);

        ret = execve(path, args, NULL);
      
    } else if (pid > 0) {
      printf("parent process in loop: [%d]\n", getpid());
      if((pid = wait(&status)) < 0){
        perror("wait");
      }
      // exit parent!!!!
      _exit(1);
      
    } else {
      perror("fork failed");
    }

    free(path);

    if (ret != -1) {
      break;
    }
  }
  return ret != -1;
}


bool excute_external_test_fullpath(char *path, char **args) {
  int status, ret;
  pid_t pid = fork();

  if (pid == 0) {
    ret = execve(path, args, NULL);
  } else if (pid > 0) {
    if((pid = wait(&status)) < 0){
      perror("wait");
      _exit(1);
    }
    // if (WIFEXITED(status)){
    //   ret = WEXITSTATUS(status);
    // }
  } else {
    perror("fork failed");
  }
  // printf("%d\n",ret);
  return ret != -1; 
}


void excute_external(char **args, char *GLOBAL_PATH) {
  char *pwd;
  pwd = pwd_command();
      
  if (excute_external_test_fullpath(args[0], args)) {
    // printf("entering full");
    free(pwd);
    return;
  } else if (excute_external_test_fullpath(pwd, args)) {
    // printf("entering pwd");
    free(pwd);
    return;
  } else {
    // printf("entering search");
    char **tests = (char**) calloc(LISTEXTSIZE, sizeof(char*));
    tokenize(GLOBAL_PATH, ":", tests);
    bool ret = excute_external_looptest(tests, args);
    free(tests);
    free(pwd);
    if (ret) return;
  }

  printf("Dragonshell: Cannot excute this command\n");
  
}

void excute_internal(char **args, char *GLOBAL_PATH) {
  pid_t pid = fork();
  int status;

  char *command = args[0];
  if (pid == 0) {
    //child process
    if (strcmp(command, "pwd") == 0) {
      char *ret;
      ret = pwd_command();
      printf("%s\n", ret);
      free(ret);
    } else if (strcmp(command, "cd") == 0) {
      cd_command(args[1]);
    } else if (strcmp(command, "$PATH") == 0) {
      printf("Current PATH: %s\n", GLOBAL_PATH);
    } else if ((strcmp(command, "a2path") == 0)) {
      if (args[1]) {
        a2path(args[1], GLOBAL_PATH);
      }
    }
    _exit(1);

  } else if (pid > 0){
    // parent process
    if((pid = wait(&status)) < 0){
      perror("wait");
      _exit(1);
    }

  } else {
    perror("fork failed");
  }
}

// entey for exc single process
void analyze_single_command(char *line, char *GLOBAL_PATH) { 
  char **args = (char**) calloc(LISTEXTSIZE, sizeof(char*));
  tokenize(line, " ", args);

  char *command = args[0];
  if (strcmp(command, "pwd") == 0 | strcmp(command, "cd") == 0 | (strcmp(command, "a2path") == 0) | strcmp(command, "$PATH") == 0) {
    excute_internal(args, GLOBAL_PATH);
    return;
  }
  if ((strcmp(command, "exit") == 0)) {
    _exit(0);
  } else {
    excute_external(args, GLOBAL_PATH);
  }
  
  free(args);
}

void analyze_multiple_command(char *line) {
  char **args = (char**) calloc(LISTEXTSIZE, sizeof(char*));
  tokenize(line, ";", args);
}


char *trimspace(char *str){
  // https://stackoverflow.com/questions/122616/how-do-i-trim-leading-trailing-whitespace-in-a-standard-way
  char *end;
  // Trim leading space
  while(isspace((unsigned char)*str)) str++;
  if(*str == 0)  // All spaces?
    return str;
  // Trim trailing space
  end = str + strlen(str) - 1;
  while(end > str && isspace((unsigned char)*end)) end--;
  // Write new null terminator character
  end[1] = '\0';
  return str;
}


void welcome() {
  printf("Welcome to Dragon Shell!\n");
}

int main(int argc, char **argv) {
  // print the string prompt without a newline, before beginning to read
  // tokenize the input, run the command(s), and print the result
  // do this in a loop
  char *line, GLOBAL_PATH[TEXTSIZE];

  welcome();
  init_globalpath(GLOBAL_PATH);
  for (;;){
    char line[TEXTSIZE];
    scan_line(line);
    analyze_single_command(line, GLOBAL_PATH);


  }

  return 0;
}