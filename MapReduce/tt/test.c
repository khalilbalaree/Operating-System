#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void Map(char *file_name) {
    FILE *fp = fopen(file_name, "r");
    assert(fp != NULL);
    char *line = NULL;
    size_t size = 0;
    while (getline(&line, &size, fp) != -1) {
        char *token, *dummy = line;
        while ((token = strsep(&dummy, " \t\n\r")) != NULL)
            printf("%s\n", token);
            // MR_Emit(token, "1");
    }
    free(line);
    fclose(fp);
}

int main(int argc, char *argv[]) {
    char **filename = &argv[1];
    for (int i=0; i < argc-1;i++) {
        printf("%s\n",filename[i]);
    }
//     Map("in.txt");
}