/*
=================================================
ASSIGNMENT - 1 : Multi-process Applications
=================================================
NAME: Tuhin Mondal
ROLL: 22CS10087
-------------------------------------------------
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define FILENAME "foodep.txt"
#define DONEFILE "done.txt"
#define MAXFILES 1000

int depfilecnt;
int depfile[MAXFILES];

char check(int index)
{
    char ch;
    FILE *file = fopen(DONEFILE, "r");
    if (file == NULL) exit(EXIT_FAILURE);
    for (int i = 0; i <= index; i++)
    {
        ch = fgetc(file);
        if (ch == EOF)
        {
            printf("Index out of bounds.\n");
            fclose(file);
            return '\0';
        }
    }
    fclose(file);
    return ch;
}

void set(int index)
{
    FILE *file = fopen(DONEFILE, "r+");
    if (file == NULL) exit(EXIT_FAILURE);
    for (int i = 0; i <= index; i++)
    {
        if (fgetc(file) == EOF)
        {
            printf("Index out of bounds.\n");
            fclose(file);
            return;
        }
    }
    fseek(file, -1, SEEK_CUR);
    fputc('1', file);
    fclose(file);
}

void getDep(int key)
{
    depfilecnt = 0;
    char line[2048];
    FILE *file = fopen(FILENAME, "r");
    if (file == NULL) exit(EXIT_FAILURE);

    while (fgets(line, sizeof(line), file))
    {
        int x;
        if (sscanf(line, "%d:", &x) == 1 && key == x)
        {
            char *ptr = strchr(line, ':');
            if (ptr == NULL) continue; 
            ptr++;

            while (*ptr == ' ') ptr++; 
            while (sscanf(ptr, "%d", &depfile[depfilecnt]) == 1)
            {
                depfilecnt++;
                while (*ptr >= '0' && *ptr <= '9') ptr++;
                while (*ptr == ' ') ptr++;
            }
            break;
        }
    }

    fclose(file);
    return;
}

int main(int argc, char* argv[])
{
    int start;
    if (argc == 1) { printf("Usage: %s <node>\n", argv[0]); exit(EXIT_FAILURE); }
    start = atoi(argv[1]);

    if (argc == 2) 
    {
        FILE *fp = fopen(FILENAME, "r");
        if (fp == NULL) exit(EXIT_FAILURE); 

        int n = 0;
        fscanf(fp, "%d", &n);
        FILE *visited = fopen(DONEFILE, "w");
        if (visited == NULL) exit(EXIT_FAILURE);
        for (int i = 0; i < n; i++) fputc('0', visited);
        fclose(visited);
        fclose(fp);
    }
    
    getDep(start);
    set(start - 1);
    for (int i = 0; i < depfilecnt; i++)
    {
        if (check(depfile[i] - 1) == '0')
        {
            pid_t pid = fork();
            if (pid == 0)
            {
                char depfile_str[10];
                snprintf(depfile_str, sizeof(depfile_str), "%d", depfile[i]);
                execlp("./rebuild", "./rebuild" , depfile_str, "x", NULL);
            }
            else wait(NULL);
        }
    }
    printf("foo%d rebuilt", start);

    if (depfilecnt != 0) {
        printf(" from foo%d", depfile[0]);
        for (int i = 1; i < depfilecnt; i++) printf(", foo%d", depfile[i]);
    }
    printf("\n");
    return 0;
}