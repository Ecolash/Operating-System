#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#define NUM_BLOCKS 9

void displayHelp() {
    printf("Commands supported:\n\n");
    printf("\tn          Start new game\n");
    printf("\tp b c d    Put digit d [1-9] at cell c [0-8] of block b [0-8]\n");
    printf("\ts          Show solution\n");
    printf("\th          Print this help message\n");
    printf("\tq          Quit\n\n");

    printf("Numbering scheme for blocks and cells\n");
    printf("\t+---+---+---+\n");
    printf("\t| 0 | 1 | 2 |\n");
    printf("\t+---+---+---+\n");
    printf("\t| 3 | 4 | 5 |\n");
    printf("\t+---+---+---+\n");
    printf("\t| 6 | 7 | 8 |\n");
    printf("\t+---+---+---+\n");
}

void createPipe(int pipes[NUM_BLOCKS][2], int index) {
    if (pipe(pipes[index]) == -1) {
        perror("Pipe creation failed");
        exit(EXIT_FAILURE);
    }
}

void forkAndExec(int pipes[NUM_BLOCKS][2], int index, const char *geometry, const char *title) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("Fork failed");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        for (int i = 0; i < NUM_BLOCKS; i++) {
            close(pipes[i][0]);
            if (i != index) close(pipes[i][1]);
        }

        // Redirect stdin to the read end of the pipe
        dup2(pipes[index][0], STDIN_FILENO);
        close(pipes[index][0]);

        char block_no_str[4], pipe_read_fd_str[4], pipe_write_fd_str[4];
        sprintf(block_no_str, "%d", index);
        sprintf(pipe_read_fd_str, "%d", pipes[index][0]);
        sprintf(pipe_write_fd_str, "%d", pipes[index][1]);

        execlp("xterm", "xterm",
               "-T", title,
               "-fa", "Monospace", "-fs", "12",
               "-geometry", geometry,
               "-bg", "#000000", "-fg", "cyan",
               "-e", "./block", block_no_str, pipe_read_fd_str, pipe_write_fd_str,
               NULL);

        perror("execlp failed");
        exit(EXIT_FAILURE);
    }
}

int main() {
    int pipes[NUM_BLOCKS][2];
    displayHelp();

    const char *geometries[NUM_BLOCKS] = {
        "28x13+900+50", "28x13+1200+50", "28x13+1500+50",
        "28x13+900+350", "28x13+1200+350", "28x13+1500+350",
        "28x13+900+650", "28x13+1200+650", "28x13+1500+650"
    };

    const char *titles[NUM_BLOCKS] = {
        "Block 0", "Block 1", "Block 2",
        "Block 3", "Block 4", "Block 5",
        "Block 6", "Block 7", "Block 8"
    };

    for (int i = 0; i < NUM_BLOCKS; i++) {
        createPipe(pipes, i);
        forkAndExec(pipes, i, geometries[i], titles[i]);
    }

    char command;
    int A[9][9], S[9][9];

    while (1) {
        printf("foodoku> ");
        scanf(" %c", &command);

        switch (command) {
            case 'h':
                displayHelp();
                break;

            case 'n':
                // Start new game logic here
                break;

            case 'p': {
                int b, c, d;
                scanf("%d %d %d", &b, &c, &d);
                if (b < 0 || b >= NUM_BLOCKS || c < 0 || c >= 9 || d < 1 || d > 9) {
                    printf("Invalid input\n");
                } else {
                    char msg[4];
                    sprintf(msg, "p %d %d", c, d);
                    // Switch stdout to the pipe of the block
                    int stdout_copy = dup(STDOUT_FILENO);
                    dup2(pipes[b][1], STDOUT_FILENO);
                    printf("%s\n", msg);
                    fflush(stdout);
                    // Restore original stdout
                    dup2(stdout_copy, STDOUT_FILENO);
                    close(stdout_copy);
                }
                break;
            }

            case 's':
                // Show solution logic here
                break;

            case 'q':
                for (int i = 0; i < NUM_BLOCKS; i++) {
                    // Switch stdout to the pipe of the block
                    int stdout_copy = dup(STDOUT_FILENO);
                    dup2(pipes[i][1], STDOUT_FILENO);
                    printf("q\n");
                    fflush(stdout);
                    // Restore original stdout
                    dup2(stdout_copy, STDOUT_FILENO);
                    close(stdout_copy);
                }
                for (int i = 0; i < NUM_BLOCKS; i++) {
                    wait(NULL);
                }
                return 0;

            default:
                printf("Unknown command! Use h for supported commands\n");
                break;
        }
    }
    return 0;
}
