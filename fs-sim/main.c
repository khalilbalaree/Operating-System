#include "FileSystem.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

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

int analyze_command(char **args, char *line) {
    int size_args = 0;
    while (args[size_args]) {
        size_args += 1;
    }
    if (strcmp(args[0], "M") == 0) {
        if (size_args != 2) {
            return 0;
        }
        fs_mount(args[1]);
    } else if (strcmp(args[0], "C") == 0) {
        if (size_args != 3 || strlen(args[1]) > 5) {
            return 0;
        }
        // printf("%s\n", args[1]);
        int size = (int) strtol(args[2], (char **)NULL, 10);
        if (size < 0 || size > 127) {
            return 0;
        }
        fs_create(args[1], size);
    } else if (strcmp(args[0], "D") == 0) {
        if (size_args != 2 || strlen(args[1]) > 5) {
            return 0;
        }
        fs_delete(args[1]);
    } else if (strcmp(args[0], "R") == 0) {
        if (size_args != 3 || strlen(args[1]) > 5) {
            return 0;
        }
        fs_read(args[1], (int) strtol(args[2], (char **)NULL, 10));
    } else if (strcmp(args[0], "W") == 0) {
        if (size_args != 3 || strlen(args[1]) > 5) {
            return 0;
        }
        fs_write(args[1], (int) strtol(args[2], (char **)NULL, 10));
    } else if (strcmp(args[0], "B") == 0) {
        line++;
        char *new_line = trimspace(line);
        if (strlen(new_line) == 0) {
            return 0;
        }
        fs_buff((uint8_t*) new_line);
    } else if (strcmp(args[0], "L") == 0) {
        if (size_args != 1) {
            return 0;
        }
        fs_ls();
    } else if (strcmp(args[0], "Y") == 0) {
        if (size_args != 2 || strlen(args[1]) > 5) {
            return 0;
        }
        fs_cd(args[1]);
    } else if (strcmp(args[0], "E") == 0) {
        if (size_args != 3 || strlen(args[1]) > 5) {
            return 0;
        }
        fs_resize(args[1], strtol(args[2], (char **)NULL, 10));
    } else if (strcmp(args[0], "O") == 0) {
        if (size_args != 1) {
            return 0;
        }
        fs_defrag();
    }

    return 1;
}

int main(int argc, char *argv[]) {
    if (argv[1] == NULL) {
        printf("No input file!\n");
        return 0;
    }
    char const* const fileName = argv[1];
    if (argv[2] != NULL) {
        printf("Too many input files!\n");
        return 0;
    }
    FILE* file = fopen(fileName, "r");
    if (file == NULL) {
        printf("Input file not exist, exiting...\n");
        return 0;
    }

    char *line = (char*) malloc (1048);
    int line_num = 1;
    
    while (fgets(line, 1048, file)) {
        if (strcmp(line, "\n") == 0) {
            fprintf(stderr, "Command Error: %s, %d\n", fileName, line_num);
            line_num += 1;
            continue;
        }
        char **args = (char**) calloc (256, sizeof(char*));
        char *line_cpy = (char*) malloc(strlen(line)+1);
        strcpy(line_cpy, line); 
        tokenize(trimspace(line), " ", args);
        if (!analyze_command(args, trimspace(line_cpy))) {
            fprintf(stderr, "Command Error: %s, %d\n", fileName, line_num);
        }
        free(args);
        free(line_cpy);
        line_num += 1;
    }

    free(line);
    fclose(file);

    freeEverything();

    return 0;
}


