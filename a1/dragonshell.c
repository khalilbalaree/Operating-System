#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>
// #include <signal.h>

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

void pwd_command() {
  char *buffer = (char*) calloc(TEXTSIZE, sizeof(char));
  if (getcwd(buffer, TEXTSIZE) != NULL) {
    printf("%s\n", buffer);
  } else {
    printf("pwd command failed\n");
  }
  free(buffer);
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

void show_path() {
  char *env;
  env = getenv("PATH");
  if (env != NULL) {
    printf("Current PATH: %s\n", env);
  } else {
    printf("No $PATH found");
  }
}

void get_path_arr(char **paths) {
  char *env;
  env = getenv("PATH");
  tokenize(env, ":", paths);
}

void a2path(char *path) {
  char *env, **args, *newpath;
  int start;
  args = (char**) calloc(LISTEXTSIZE, sizeof(char*));
  newpath = (char*) calloc(TEXTSIZE, sizeof(char));
  tokenize(path, ":", args);
  if (strcmp(args[0], "$PATH") == 0) {
    env = getenv("PATH");
    if (env == NULL) {
      printf("No $PATH found\n");
      return;
    }
    strcat(newpath, env);
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
  printf("%s\n", newpath);
  free(args);
  free(newpath);
}


bool excute_external(char *line) { 
  int status, ret;
  pid_t pid = fork();
  if (pid == 0) {
    char *argv[] = {"bash", "-c", line,  NULL};
    execve("/bin/bash", argv, NULL);
  } else if (pid > 0) {
    if((pid = wait(&status)) < 0){
      perror("wait");
      _exit(1);
    }
    if (WIFEXITED(status)){
      ret = WEXITSTATUS(status);
    }
  } else {
    perror("fork failed");
  }
  // printf("%d\n",ret);
  return ret == 0; 
}

void excute_internal(char **args) {
  pid_t pid = fork();
  int status;

  char *command = args[0];
  if (pid == 0) {
    if (strcmp(command, "pwd") == 0) {
      pwd_command();
    } else if (strcmp(command, "cd") == 0) {
      cd_command(args[1]);
    } else if (strcmp(command, "$PATH") == 0) {
      show_path();
    } else if ((strcmp(command, "a2path") == 0)) {
      if (args[1]) {
        a2path(args[1]);
      }
    }
    _exit(1);
  } else if (pid > 0){
    if((pid = wait(&status)) < 0){
      perror("wait");
      _exit(1);
    }
  } else {
    perror("fork failed");
  }
}

// entey for exc single process
void analyze_single_command(char *line) { 
  char *cpline = (char*) calloc(TEXTSIZE, sizeof(char));
  char **args = (char**) calloc(LISTEXTSIZE, sizeof(char*));
  strcpy(cpline, line);
  tokenize(line, " ", args);

  char *command = args[0];
  if (strcmp(command, "pwd") == 0 | strcmp(command, "cd") == 0 | (strcmp(command, "a2path") == 0) | strcmp(command, "$PATH") == 0) {
    excute_internal(args);
    return;
  }
  if ((strcmp(command, "exit") == 0)) {
    _exit(0);
  } else {
    excute_external(cpline);     
  }
  
  free(args);
  free(cpline);
}

void analyze_multiple_command(char *line) {
  char **args = (char**) calloc(LISTEXTSIZE, sizeof(char*));
  tokenize(line, ";", args);


  
}


void analyze_line(char *line) {
  char **args = (char**) calloc(LISTEXTSIZE, sizeof(char*));
  tokenize(line, " ", args);
  printf("%s\n", args[0]);
  printf("%s\n", args[1]);

  free(args);
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
  char *line;

  welcome();
  for (;;){
    char line[TEXTSIZE];
    scan_line(line);
    analyze_single_command(line);


  }

  return 0;
}