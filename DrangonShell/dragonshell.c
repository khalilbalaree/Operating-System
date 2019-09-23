#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#define TEXTSIZE 500
#define LISTEXTSIZE 100

int skip;

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

/**
 * @return: 1: normal
 * @return: 0: ignore
 * @return: -1: exit program
 */ 
int scan_line(char *str) {
	printf("dragonshell > ");
  if (fgets(str, TEXTSIZE, stdin)) {
    size_t size = strlen(trimspace(str));
    if (size > 0) {
      return 1;
    } else {
      return 0;
    }
  } else if (feof(stdin)){
    return -1;
  } else {
    return 0;
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

bool find_path_full(char *path){
  if (access(path, F_OK ) != -1) {
    return true;
  } else {
    return false;
  }
}

bool find_path_loop(char **tests, char *command, char *path) {
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

void excute_external_test_fullpath(char *path, char **args, int bg) {
  // printf("excuting external...\n");
  pid_t ppid = getpid();
  // printf("bg :%d\n", bg);
  pid_t pid = fork();

  if (pid == 0) {
    // printf("child: %d\n", getpid());
    if (bg == 1) { 
      // reference: https://stackoverflow.com/questions/26453624/hide-terminal-output-from-execve
      int fd = open("/dev/null", O_WRONLY);
      dup2(fd, 1);    /* make stdout a copy of fd (> /dev/null) */
      dup2(fd, 2);    /* ...and same with stderr */
      close(fd);      /* close fd */
    }
    execve(path, args, NULL);
    _exit(1);
  } else if (pid > 0) {
    // printf("parent: %d\n", ppid);
    if (bg == 0) {
      waitpid(pid, 0, 0);
    } else {
      printf("PID %d is running in the background\n", pid);
    }
    
  } else {
    perror("fork failed");
  }
}

// create a new precess
void excute_external(char **args, char *GLOBAL_PATH, int bg) {
  // printf("external: command: %s\n", args[0]);

  // 1. try to excute full path
  if (find_path_full(args[0])) {
    excute_external_test_fullpath(args[0], args, bg);
    return;
  }
  // 2. try to excute from pwd
  char *pwd = pwd_command();
  char *path_pwd = (char*) calloc(TEXTSIZE, sizeof(char));
  strcpy(path_pwd, pwd);
  strcpy(path_pwd,args[0]);

  if (find_path_full(path_pwd)) {
    excute_external_test_fullpath(pwd, args, bg);
    free(pwd);free(path_pwd);
    return;
  }
  free(pwd);free(path_pwd);

  // 3. try to excute from shell path
  char **tests = (char**) calloc(LISTEXTSIZE, sizeof(char*));
  char *path = (char*) calloc(TEXTSIZE, sizeof(char));
  char *globalpath_cpy = (char*) calloc(TEXTSIZE, sizeof(char));
  // keep GLOBAL_PATH unchange
  strcpy(globalpath_cpy, GLOBAL_PATH);
  tokenize(globalpath_cpy, ":", tests);
  if (find_path_loop(tests, args[0], path)) {
    // printf("find path ok\n");
    excute_external_test_fullpath(path, args, bg);
    free(tests);free(path);free(globalpath_cpy);
    return;
  }
  free(tests);free(path);free(globalpath_cpy);

  // 4. no path find then printf
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

// level 4
void analyze_single_command(char *line, char *GLOBAL_PATH) { 
  char **args = (char**) calloc(LISTEXTSIZE, sizeof(char*));
  tokenize(line, " ", args);

  char *command = args[0];
  if (strcmp(command, "pwd") == 0 | strcmp(command, "cd") == 0 | (strcmp(command, "a2path") == 0) | strcmp(command, "$PATH") == 0 | strcmp(command, "exit") == 0) {
    excute_internal(args, GLOBAL_PATH);
  } else {
    excute_external(args, GLOBAL_PATH, 0);
  }
  free(args);
}


//level 3.5
void piping_process(char *org, char *GLOBAL_PATH, char *dst) {
  int p[2], status;
  pipe(p);
  // printf("c1: %s   c2: %s\n", org, dst);

  if (fork() == 0) { // process 1
    close(fileno(stdout));
    dup(p[1]);
    close(p[0]);
    close(p[1]); 

    analyze_single_command(org, GLOBAL_PATH);
    _exit(1);
  } 
  if (fork() == 0) { // process 2
    close(fileno(stdin));
    dup(p[0]);
    close(p[1]);
    close(p[0]);

    analyze_single_command(dst, GLOBAL_PATH);
    _exit(1);
  } 
  close(p[0]);
  close(p[1]);
  wait(&status);
  wait(&status);
}

// level 3
void analyze_piping_command(char *line, char *GLOBAL_PATH) {
  char **args = (char**) calloc(LISTEXTSIZE, sizeof(char*));
  tokenize(line, "|", args);
  if (args[2] != NULL) {
    printf("Dragonshell: piping: Too many arguments\n");
  } else if (args[1] != NULL) { //piping...
    piping_process(trimspace(args[0]), GLOBAL_PATH, trimspace(args[1]));
  } else {
    analyze_single_command(trimspace(line), GLOBAL_PATH);
  }
  free(args);
}

// level 2.5
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

  analyze_piping_command(line, GLOBAL_PATH);

  fflush(stdout); 
  close(fd);
  dup2(out, fileno(stdout));
  close(out);
}

// level 2
void analyze_redirect_command(char *line, char *GLOBAL_PATH) {
  char **args = (char**) calloc(LISTEXTSIZE, sizeof(char*));
  tokenize(line, ">", args);
  if (args[2] != NULL) {
    printf("Dragonshell: redirecting: Too many arguments\n");
  } else if (args[1] != NULL) { //redirecting...
    redirecting(trimspace(args[0]), GLOBAL_PATH, trimspace(args[1]));
  } else {
    analyze_piping_command(trimspace(line), GLOBAL_PATH);
  }
  free(args);
}


// level 1
void analyze_multiple_threads(char *line,char *GLOBAL_PATH) {
  if (strchr(line, ';')) { // multiple 
    // printf("entering multiple...\n");
    char **commands = (char**) calloc(LISTEXTSIZE, sizeof(char*));
    tokenize(line, ";", commands);
    for (size_t i=0; commands[i]; ++i){
      analyze_redirect_command(trimspace(commands[i]), GLOBAL_PATH);
    }
    free(commands);
  } else { // single
    analyze_redirect_command(trimspace(line), GLOBAL_PATH);
  }
}

// level 0.5
void backgrounding(char *line, char *GLOBAL_PATH) {
  // printf("bg func: %s", line);
  char **args = (char**) calloc(LISTEXTSIZE, sizeof(char*));
  tokenize(line, " ", args);

  char *command = args[0];
  if (strcmp(command, "pwd") == 0 | strcmp(command, "cd") == 0 | (strcmp(command, "a2path") == 0) | strcmp(command, "$PATH") == 0 | strcmp(command, "exit") == 0) {
    printf("Drangonshell: &: builtin-commands not supported\n");
  } else {
    excute_external(args, GLOBAL_PATH, 1);
  }

  free(args);
}

// level 0
void analyze_background_thread(char *line, char *GLOBAL_PATH) {
  if (strchr(line, '&')) {
    if (strchr(line, '|') != NULL| strchr(line, '>') != NULL| strchr(line, ';') != NULL) {
      printf("Dragonshell: &: only one command in background\n");
    } else {
      int size = strlen(line);
      line[size -1] = '\0';
      backgrounding(line, GLOBAL_PATH);
    }
  } else {
    analyze_multiple_threads(line, GLOBAL_PATH);
  }
}

// reference: signal1.c from eclass
void signal_callback_handler(int signum) {
	// printf("Caught signal!\n");
	printf("\n");
	skip = 1;
}

// reference: signal1.c from eclass
void catch_exit_signal() {
  struct sigaction sa;
	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);
	sa.sa_handler = signal_callback_handler;
	sigaction(SIGINT, &sa, NULL);
  sigaction(SIGTSTP, &sa, NULL);
  sigaction(SIGQUIT, &sa, NULL);
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
  char GLOBAL_PATH[TEXTSIZE],line[TEXTSIZE];
  int ret;

  welcome();
  init_globalpath(GLOBAL_PATH);
  catch_exit_signal();
  // catch_child_exit_signal();

  while (1) { 
    skip = 0;
    ret = scan_line(line);

    // printf("%s\n", line);

    if (ret == -1) {
      printf("\nDragonshell: Exit with ctrl+D\n");
      return 0;
    } else if (ret == 0) {
      continue;
    }

    if (!skip) {
      // analyze_multiple_threads(line, GLOBAL_PATH);
      analyze_background_thread(trimspace(line), GLOBAL_PATH);
    }
      
  }
  return 0;
}