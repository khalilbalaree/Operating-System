// #include "dragonshell.h"
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
// #include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <signal.h>

#define TEXTSIZE 500
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

/**
 * Use for: Trimming leading and trailing whitespace from a C string
 * Reference : https://stackoverflow.com/questions/122616/how-do-i-trim-leading-trailing-whitespace-in-a-standard-way
 */
char *trimspace(char *str){
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

// TODO : PATH var pass back!!!!!!!!!!!!!!!!!!!!!!!!!
char *a2path(char *path, char *GLOBAL_PATH) {
  char *env, **args, *newpath;
  int start;
  args = (char**) calloc(LISTEXTSIZE, sizeof(char*));
  newpath = (char*) calloc(TEXTSIZE, sizeof(char));
  tokenize(path, ":", args);
  if (strcmp(args[0], "$PATH") == 0) {
    strcpy(newpath, GLOBAL_PATH);
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
  free(args);
  return newpath;
}

// void redirecting(char *file, int mode) {
//   // try redirecting
//   // mode: 0 output to screen
//   // mode: 1 redirecting to file
//   if (mode) {
//     int fd = open(file, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
//     dup2(fd, fileno(stdout)); //stdout
//     dup2(fd, fileno(stderr)); //stderr
//     close(fd);
//   }
//   // end trying
// }

bool find_path(char **tests, char *command, char *path) {
  int ok;
  for (size_t i=0; tests[i]; ++i){
    char *path_try = (char*) calloc(TEXTSIZE, sizeof(char));
    strcpy(path_try, tests[i]);
    if (strcmp(&path_try[strlen(path_try)-1], "/") != 0){
      strcat(path_try, "/");
    }
    strcat(path_try, command);
    // printf("try path: %s\n", path_try);
    if (access(path_try, F_OK ) != -1) {
      // printf("try path: find! %s\n", path_try);
      ok = 1;
      strcpy(path, path_try);
      free(path_try);
      break;
    }
    free(path_try);
  }
  return ok == 1;
}

bool excute_external_test_fullpath(char *path, char **args) {
  int status, ret;
  int p[2];
  // create pipe descriptors
  pipe(p);
  pid_t pid = fork();

  if (pid == 0) {
    ret = execve(path, args, NULL);
    close(p[0]);
    write(p[1], &ret, sizeof(ret));
    close(p[1]);
    _exit(1);
  } else if (pid > 0) {
    if((pid = wait(&status)) < 0){
      perror("wait");
      _exit(1);
    }
    close(p[1]);
    read(p[0], &ret, sizeof(ret));
    close(p[0]);
  } else {
    perror("fork failed");
  }
  return ret != -1; 
}

// create a new precess
void excute_external(char **args, char *GLOBAL_PATH) {
  char *pwd;
  pwd = pwd_command();
  // printf("external: command: %s\n", args[0]);
      
  if (excute_external_test_fullpath(args[0], args)) {
    // printf("entering full\n");
    free(pwd);
    return;
  } else if (excute_external_test_fullpath(pwd, args)) {
    // printf("entering pwd\n");
    free(pwd);
    return;
  } else {
    // printf("entering search : globalpath: %s\n", GLOBAL_PATH);
    char **tests = (char**) calloc(LISTEXTSIZE, sizeof(char*));
    char *path = (char*) calloc(TEXTSIZE, sizeof(char));
    char *globalpath_cpy = (char*) calloc(TEXTSIZE, sizeof(char));
    strcpy(globalpath_cpy, GLOBAL_PATH);
    tokenize(globalpath_cpy, ":", tests);
    if (find_path(tests, args[0], path)) {
      // printf("find path ok\n");
      if (excute_external_test_fullpath(path, args)) {
        free(tests);free(path);free(pwd);free(globalpath_cpy);
        return;
      }
    }
    free(tests);free(path);free(pwd);free(globalpath_cpy);
  }
  printf("Dragonshell: %s :command not found\n", args[0]); 
}

// runing in the shell
void excute_internal(char **args, char *GLOBAL_PATH) {
  char *command = args[0];
  // printf("internal: get command: %s\n", command);

  if (strcmp(command, "pwd") == 0) {
    if (args[1]) {
      printf("Dragonshell: pwd: Too many arguments\n");
    } else {
      char *ret = pwd_command();
      printf("%s\n", ret);
      free(ret);
    }
  } else if (strcmp(command, "$PATH") == 0) {
    if (args[1]) {
      printf("Dragonshell: $PATH: Too many arguments\n");
    } else {
      printf("Current PATH: %s\n", GLOBAL_PATH);
    }
  } else if ((strcmp(command, "a2path") == 0)) {
    if (args[1]) {
      char *path;
      path = a2path(args[1], GLOBAL_PATH);
      strcpy(GLOBAL_PATH, path);
      free(path);
    } else if (args[2]) {
      printf("Dragonshell: a2path: Too many arguments\n");
    } else {
      printf("Dragonshell: a2path: No path detected\n");
    }
  } else if (strcmp(command, "cd") == 0) {
    if (args[2] != NULL) {
      printf("Dragonshell: cd :Too many arguments\n");
    } else {
      cd_command(args[1]);
    }
  } else if (strcmp(command, "exit") == 0) {
    _exit(0);
  } 
}

// level 2.5
void analyze_single_command(char *line, char *GLOBAL_PATH) { 
  char **args = (char**) calloc(LISTEXTSIZE, sizeof(char*));
  tokenize(line, " ", args);

  char *command = args[0];
  if (strcmp(command, "pwd") == 0 | strcmp(command, "cd") == 0 | (strcmp(command, "a2path") == 0) | strcmp(command, "$PATH") == 0 | strcmp(command, "exit") == 0) {
    excute_internal(args, GLOBAL_PATH);
  } else {
    excute_external(args, GLOBAL_PATH);
  }
  free(args);
}

//level 2
void redirecting(char *line, char *GLOBAL_PATH, char *filename) {
  int fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
  if (fd == -1) { 
    perror("opening file"); 
    return; 
  }

  int out = dup(fileno(stdout));
  if (-1 == dup2(fd, fileno(stdout))) {
    perror("cannot redirect stdout"); 
    return; 
  }

  analyze_single_command(line, GLOBAL_PATH);

  fflush(stdout); 
  close(fd);
  dup2(out, fileno(stdout));
  close(out);
}


// level 1
void analyze_redirect_command(char *line, char *GLOBAL_PATH) {
  char **args = (char**) calloc(LISTEXTSIZE, sizeof(char*));
  tokenize(line, ">", args);
  if (args[2] != NULL) {
    printf("Dragonshell: Not supporting for output redirection");
  } else if (args[1] != NULL) { //redirecting...
    redirecting(trimspace(args[0]), GLOBAL_PATH, trimspace(args[1]));
  } else {
    analyze_single_command(trimspace(line), GLOBAL_PATH);
  }
  free(args);
}

// level 0
void analyze_multiple_command(char *line) {
  char **args = (char**) calloc(LISTEXTSIZE, sizeof(char*));
  tokenize(line, ";", args);
}


void welcome() {
  printf("Welcome to Dragon Shell!\n");
}

void init_globalpath(char *GLOBAL_PATH) {
  strcpy(GLOBAL_PATH, "/bin/:/usr/bin/");
}

int main(int argc, char **argv) {
  // print the string prompt without a newline, before beginning to read
  // tokenize the input, run the command(s), and print the result
  // do this in a loop
  char *line, GLOBAL_PATH[TEXTSIZE];

  welcome();
  init_globalpath(GLOBAL_PATH);

  while (1) {
    char line[TEXTSIZE];
    scan_line(line);
    analyze_redirect_command(line, GLOBAL_PATH);

  }

  return 0;
}