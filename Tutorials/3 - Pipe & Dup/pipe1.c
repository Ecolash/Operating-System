#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main() {
    int fd[2];
    pipe(fd);
    if (fork() == 0) {
        close(fd[0]);
        int x;
        printf("Enter a number: ");
        scanf("%d", &x);
        write(fd[1], &x, sizeof(x));
        close(fd[1]);
    } else {
        close(fd[1]);
        int y;
        read(fd[0], &y, sizeof(y));
        printf("Parent received: %d\n", y);
        close(fd[0]);
    }
    return 0;
}