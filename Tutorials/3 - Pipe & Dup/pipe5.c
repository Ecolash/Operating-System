#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_NUM 1000

int binary_search_pipe(int low, int high, int secret_number) {
    int pipe_fd[2];
    pipe(pipe_fd);
    pid_t pid = fork();

    if (pid == 0) {
        close(pipe_fd[1]);
        int guess;
        while (read(pipe_fd[0], &guess, sizeof(int)) > 0)
            if (guess == secret_number) exit(EXIT_SUCCESS);
            else if (guess < secret_number) printf("%d is too low.\n", guess);
            else printf("%d is too high.\n", guess);
        close(pipe_fd[0]);
        exit(EXIT_FAILURE);
    }

    close(pipe_fd[0]);
    int mid;
    while (low <= high) {
        mid = (low + high) / 2;
        printf("Guessing %d\n", mid);
        write(pipe_fd[1], &mid, sizeof(int));
        if (mid == secret_number) break;
        else if (mid < secret_number) low = mid + 1;
        else high = mid - 1;
    }

    close(pipe_fd[1]);
    wait(NULL);
    return mid;
}

int main() {
    int secret_number;
    printf("Enter the secret number (between 1 and %d): ", MAX_NUM);
    scanf("%d", &secret_number);
    if (secret_number < 1 || secret_number > MAX_NUM) return EXIT_FAILURE;
    binary_search_pipe(1, MAX_NUM, secret_number);
    return 0;
}
