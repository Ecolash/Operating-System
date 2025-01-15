#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#define PLAYING   10001
#define CATCHMADE 10002
#define CATCHMISS 10003
#define OUTOFGAME 10004

const char s0[8] = " ....  ";
const char s1[8] = " CATCH ";
const char s2[8] = " MISS  ";
const char s3[8] = "       ";


#define FILENAME "childpid.txt"
#define FILENAME2 "dummycpid.txt"

pid_t *siblings;
int status;
int self, n;

void catch_signal(int sig)
{
    int catch = (rand() % 10 < 8)? CATCHMADE : CATCHMISS;
    status = catch;

    // if (catch == CATCHMADE) printf("[+] Child %d: I caught the ball\n", self);
    // else printf("[+] Child %d: I missed the ball\n", self);
    if (catch == CATCHMADE) kill(getppid(), SIGUSR1);
    else kill(getppid(), SIGUSR2);
}

void terminate()
{
    pid_t dummyPID;
    FILE *file2 = fopen(FILENAME2, "r");
    if (file2 == NULL) exit(EXIT_FAILURE);
    fscanf(file2, "%d", &dummyPID);
    fclose(file2);

    // printf("%d -> %d\n", self, dummyPID);
    printf("\n");
    kill(dummyPID, SIGINT);
    return; 
}

void print_row(int sig)
{
    switch (status)
    {
        case PLAYING:   printf("%s", s0); break;
        case CATCHMADE: printf("%s", s1); break;
        case CATCHMISS: printf("%s", s2); break;
        case OUTOFGAME: printf("%s", s3); break;
        default: break;
    }
    printf("│");
    fflush(stdout);    
    if (status == CATCHMADE) status = PLAYING;
    if (status == CATCHMISS) status = OUTOFGAME;
    if (self == n) terminate(); 
    else {
        pid_t next_pid = siblings[self];
        kill(next_pid, SIGUSR1);
    }
}

void results(int sig)
{
    if (status == PLAYING) printf("[+] Child %d:  Yay! I am the winner!\n\n", self);
    exit(EXIT_SUCCESS);
}

int main()
{
    srand(time(NULL) + getpid());
    sleep(1);
    signal(SIGINT, results);
    signal(SIGUSR1, print_row);
    signal(SIGUSR2, catch_signal);

    int cPID = getpid();
    FILE *file = fopen(FILENAME, "r");
    fscanf(file, "%d", &n); 
    siblings = (pid_t *)malloc(n * sizeof(pid_t));

    for (int i = 0; i < n; i++)
    {
        fscanf(file, "%d", &siblings[i]);
        if (siblings[i] == cPID) self = i + 1;
    }

    fclose(file);
    status = PLAYING;
    while (1) pause();
    return 0;
}