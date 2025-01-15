#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#define N_DEFAULT 10
#define FILENAME  "childpid.txt"
#define FILENAME2 "dummycpid.txt"

int ROUND_NO = 0;
int CURRENT_PLAYER = 0;
int N_PLAYING = 0;
int* OUT_OF_GAME;
pid_t FIRST_CHILD;

void signal_handler1(int sig)
{
    return;
}

void signal_handler2(int sig)
{
    OUT_OF_GAME[CURRENT_PLAYER] = 1;
    N_PLAYING--;
    return;
}

void line_break(int x)
{
    printf("+");
    for (int i = 0; i < x; i++) printf("-");
    printf("+\n");
    fflush(stdout);
}

void print_info()
{
    printf("│%3d │" , ++ROUND_NO);
    fflush(stdout);
    kill(FIRST_CHILD, SIGUSR1);
}

void print_ends(int n)
{
    int x = 8 * n + 4;
    printf("     │");
    for (int i = 1; i <= n; ++i) printf(" %3d   │", i);
    printf("\n");
    fflush(stdout);
    line_break(x);
}

int main(int argc, char *argv[]) {
    int n = N_DEFAULT;
    if (argc != 2) { printf("Usage: %s <No. of children>\n", argv[0]); exit(EXIT_FAILURE); }

    n = atoi(argv[1]);
    pid_t CPIDs[n];
    FILE *file = fopen(FILENAME, "w");
    if (file == NULL) exit(EXIT_FAILURE); 

    fprintf(file, "%d\n", n);
    for (int i = 0; i < n; i++)
    {
        pid_t pid = fork();
        switch (pid) {
            case -1: exit(EXIT_FAILURE);
            case 0:  execl("./child", "./child", NULL); 
            default: CPIDs[i] = pid; fprintf(file ,"%d\n", pid); break;
        }
    }
    fclose(file);

    N_PLAYING = n;
    FIRST_CHILD = CPIDs[0];
    OUT_OF_GAME = (int*) malloc(n * sizeof(int));

    printf("[+] Parent: %d child processes created\n", n);
    printf("[+] Parent: Waiting for child processes to read child database\n");
    for (int i = 0; i < n; i++) OUT_OF_GAME[i] = 0;
    sleep(2);

    signal(SIGUSR1, signal_handler1);
    signal(SIGUSR2, signal_handler2);
    int x = 8 * n + 4;
    print_ends(n);

    int i = n - 1;
    while(1)
    {
        i = (i + 1) % n;
        if (N_PLAYING == 1) break;
        if (OUT_OF_GAME[i] != 0) continue;

        CURRENT_PLAYER = i;
        kill(CPIDs[i], SIGUSR2);

        FILE *file2 = fopen(FILENAME2, "w");
        pid_t pid = fork();
        int dPID;

        switch (pid) {
            case -1: exit(EXIT_FAILURE);
            case 0:  execl("./dummy", "./dummy", NULL);
            default: dPID = pid; fprintf(file2 ,"%d\n", pid); break;
        }
        fclose(file2);
        print_info();

        waitpid(dPID, NULL, 0);
        line_break(x);
    }

    print_ends(n);
    for (int i = 0; i < n; i++) kill(CPIDs[i], SIGINT);
    free(OUT_OF_GAME);
    return 0;
}