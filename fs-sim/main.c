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

int analyze_command(char **args) {
    int size_args = 1;
    while (args[size_args]) {
        size_args += 1;
    }
    if (strcmp(args[0], "M") == 0) {
        if (size_args != 2) {
            return 0;
        }
        fs_mount(args[1]);
    } else if (strcmp(args[0], "C") == 0) {
        if (size_args != 3) {
            return 0;
        }
        fs_create(args[1], (int) strtol(args[2], (char **)NULL, 10));
    }

    return 1;
}

int main(int argc, char *argv[]) {
    char const* const fileName = argv[1];
    if (argv[2] != NULL) {
        printf("Too many inputfiles!\n");
        return 0;
    }
    FILE* file = fopen(fileName, "r");
    if (file == NULL) {
        printf("Input file not exist, exiting...\n");
        return 0;
    }

    char line[128];
    int line_num = 1;
    
    while (fgets(line, 128, file)) {
        if (strcmp(line, "\n") == 0) {
            fprintf(stderr, "Command Error: %s, %d\n", fileName, line_num);
            line_num += 1;
            continue;
        }
        char **args = (char**) malloc(32*sizeof(char*));
        tokenize(trimspace(line), " ", args);
        if (!analyze_command(args)){
            fprintf(stderr, "Command Error: %s, %d\n", fileName, line_num);
        }
        free(args);
        line_num += 1;
    }

    fclose(file);

    return 0;
}


