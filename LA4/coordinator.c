#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include "boardgen.c"

#define GREEN_BOLD      "\033[1;32m"
#define RED_BOLD        "\033[1;31m"
#define RESET           "\033[0m"

#define XTERM_WIDTH         24 
#define XTERM_HEIGHT        10
#define BASE_X              900      
#define BASE_Y              50      
#define SPACING_X           280   
#define SPACING_Y           250   

int _STATUS_;
int _GAME_LIVE_;
int pipes[9][2];
int A[9][9];
int S[9][9];

void send_to_block(int block, char *text)
{
    int saved_stdout = dup(STDOUT_FILENO);
    dup2(pipes[block][1], STDOUT_FILENO);
    printf("%s", text);
    fflush(stdout);
    dup2(saved_stdout, STDOUT_FILENO);
    close(saved_stdout);
    return;
}

void _handle_h_()
{
    printf("Commands supported:\n\n");
    printf("\tn          Start new game\n");
    printf("\tp b c d    Put digit d [1-9] at cell c [0-8] of block b [0-8]\n");
    printf("\ts          Show solution\n");
    printf("\th          Print this help message\n");
    printf("\tq          Quit\n\n");

    printf("Numbering scheme for blocks and cells:\n\n");
    printf("\t┌───┬───┬───┐\n");
    printf("\t│ 0 │ 1 │ 2 │\n");
    printf("\t├───┼───┼───┤\n");
    printf("\t│ 3 │ 4 │ 5 │\n");
    printf("\t├───┼───┼───┤\n");
    printf("\t│ 6 │ 7 │ 8 │\n");
    printf("\t└───┴───┴───┘\n");
}

void _handle_n_()
{
    newboard(A, S);
    _GAME_LIVE_ = 1;
    for (int i = 0; i < 9; i++)
    {
        send_to_block(i, "n");
        for (int r = 0; r < 3; r++)
        {
            for (int c = 0; c < 3; c++)
            {
                int br = (i / 3) * 3 + r;
                int bc = (i % 3) * 3 + c;
                int val = A[br][bc];

                char number[10];
                sprintf(number, " %d", val);
                send_to_block(i, number);
            }
        }
        send_to_block(i, "\n");
    }
}

void _handle_p_()
{
    int b, c, d;
    if (scanf("%d %d %d", &b, &c, &d) != 3) { printf(RED_BOLD "Error: Invalid input format\n" RESET); return; }
    if (_GAME_LIVE_ == 0){ printf(RED_BOLD "Error: Start a new game using 'n'\n" RESET); return; }

    if (b < 0 || b >= 9) { printf(RED_BOLD "Invalid block value: %d\n" RESET, b); return; }
    if (c < 0 || c >= 9) { printf(RED_BOLD "Invalid cell value: %d\n" RESET, c);  return; }
    if (d < 1 || d > 9)  { printf(RED_BOLD "Invalid digit value: %d\n" RESET, d); return; }

    char info[30];
    sprintf(info, "p %d %d\n", c, d);
    send_to_block(b, info);
}

void _handle_s_()
{
    if (_GAME_LIVE_ == 0){ printf(RED_BOLD "Error: Start a new game using 'n'\n" RESET); return; }
    for (int i = 0; i < 9; i++)
    {
        send_to_block(i, "n");
        for (int r = 0; r < 3; r++)
        {
            for (int c = 0; c < 3; c++)
            {
                int br = (i / 3) * 3 + r;
                int bc = (i % 3) * 3 + c;
                int val = S[br][bc];

                char number[10];
                sprintf(number, " %d", val);
                send_to_block(i, number);
            }
        }
        send_to_block(i, "\n");
    }
}

void _handle_q_()
{
    _STATUS_ = 0;
    for (int i = 0; i < 9; i++) send_to_block(i, "q\n");
}

void __init_block__(int n)
{
    int row = n / 3;
    int col = n % 3;
    int rn1, rn2, cn1, cn2;

    rn1 = (row * 3 + ((col + 1) % 3));
    rn2 = (row * 3 + ((col + 2) % 3));
    cn1 = (((row + 1) % 3) * 3 + col);
    cn2 = (((row + 2) % 3) * 3 + col);

    int x_pos = BASE_X + col * SPACING_X;
    int y_pos = BASE_Y + row * SPACING_Y;

    char blk_no[100], readFD[100], writeFD[100];
    snprintf(blk_no,  sizeof(blk_no), "%d", n);
    snprintf(readFD,  sizeof(readFD), "%d", pipes[n][0]);
    snprintf(writeFD, sizeof(writeFD), "%d", pipes[n][1]);

    char rn1_FD[100], rn2_FD[100];
    char cn1_FD[100], cn2_FD[100];
    snprintf(rn1_FD,  sizeof(rn1_FD), "%d", pipes[rn1][1]);
    snprintf(rn2_FD,  sizeof(rn2_FD), "%d", pipes[rn2][1]);
    snprintf(cn1_FD,  sizeof(cn1_FD), "%d", pipes[cn1][1]);
    snprintf(cn2_FD,  sizeof(cn2_FD), "%d", pipes[cn2][1]);

    char x_pos_str[6], y_pos_str[6];
    snprintf(x_pos_str, sizeof(x_pos_str), "%d", x_pos);
    snprintf(y_pos_str, sizeof(y_pos_str), "%d", y_pos);

    char geometry[20];
    char blockname[20];
    snprintf(geometry, sizeof(geometry), "%dx%d+%d+%d", XTERM_WIDTH, XTERM_HEIGHT, x_pos, y_pos);
    snprintf(blockname, sizeof(blockname), "Block %d", n);

    execlp("xterm", "xterm",
           "-T", blockname,
           "-fa", "Monospace",
           "-fs", "12",
           "-bg", "black",
           "-fg", "cyan",
           "-geometry", geometry,
           "-b", "2",
           "-e", "./block", blk_no, readFD, writeFD, rn1_FD, rn2_FD, cn1_FD, cn2_FD,
           (char *)NULL);

    printf("[-] Failed to exec xterm\n");
    exit(0);
}

int main()
{
    for (int i = 0; i < 9; i++)
    {
        int ret = pipe(pipes[i]);
        if (ret == -1) exit(EXIT_FAILURE);
    }

    for (int i = 0; i < 9; i++)
    {
        pid_t pid = fork();
        switch (pid)
        {
            case 0:  __init_block__(i); break;
            case -1: perror("[-] Fork failed"); exit(EXIT_FAILURE);
            default: break;
        }
    }

    char cmd;
    _STATUS_    = 1;
    _GAME_LIVE_ = 0;
    _handle_h_();

    while (_STATUS_)
    {
        printf(GREEN_BOLD "fooduko>" RESET " "); 
        scanf(" %c", &cmd);
        switch (cmd)
        {
            case 'h': _handle_h_();  break;
            case 'n': _handle_n_();  break;
            case 'p': _handle_p_();  break;
            case 's': _handle_s_();  break;
            case 'q': _handle_q_();  break;
            default:  printf(RED_BOLD "Invalid command. Use 'h' for help." RESET "\n");
        }
    }

    for (int i = 0; i < 9; i++) wait(NULL);    
    return 0;
}
