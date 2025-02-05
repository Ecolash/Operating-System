/* 
FUN EXERCISE
Simulating pipe Operator : date | wc -l

date  = prints the current date and time
wc -l = counts the number of lines in the input
*/

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    int fd[2];
    pipe(fd);

    if (fork() == 0) {
        close(fd[0]);
        dup2(fd[1], 1);
        close(fd[1]);
        execlp("date", "date", NULL);
    } else {
        close(fd[1]);
        dup2(fd[0], 0);
        close(fd[0]);
        execlp("wc", "wc", "-l", NULL);
    }
    return 0;
}