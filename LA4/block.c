#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define WHITE_BOLD      "\033[1;37m"
#define GREEN_BOLD      "\033[1;32m"
#define RED_BOLD        "\033[1;31m"
#define RESET           "\033[0m"

int BLK_NO;
int A[3][3]; 
int B[3][3]; 

int STDIN, STDOUT;
int R1_FD, R2_FD;
int C1_FD, C2_FD;
int solution_mode = 0;

void send(int FD, char *text)
{
    int saved_stdout = dup(STDOUT_FILENO);
    dup2(FD, STDOUT_FILENO);
    printf("%s", text);
    fflush(stdout);
    dup2(saved_stdout, STDOUT_FILENO);
    close(saved_stdout);
    return;
}

void draw_block()
{
    printf("\033[H\033[J");
    printf("\n    ┌───┬───┬───┐\n");
    for (int i = 0; i < 3; i++)
    {
        printf("    │");
        for (int j = 0; j < 3; j++)
        {
            if (A[i][j] != 0)
            {
                printf(WHITE_BOLD " %d " RESET, A[i][j]);
                printf("│");
                continue;
            }
            if (B[i][j] == 0) printf(" · ");
            else printf(" %d ", B[i][j]);
            printf("│");
        }
        printf("\n");
        if (i != 2) printf("    ├───┼───┼───┤\n");
    }
    printf("    └───┴───┴───┘\n\n");
    fflush(stdout);
}



int check_row(int row, int digit)
{
    int conflict = 0;
    char check1[100], check2[100];
    sprintf(check1, "r %d %d %d\n", row, digit, STDOUT);
    sprintf(check2, "r %d %d %d\n", row, digit, STDOUT);

    int response1, response2;
    send(R1_FD, check1);
    send(R2_FD, check2);

    scanf("%d", &response1);
    scanf("%d", &response2);
    conflict |= response1;
    conflict |= response2;
    return conflict;
}

int check_col(int col, int digit)
{

    int conflict = 0;
    char check1[100], check2[100];
    sprintf(check1, "c %d %d %d\n", col, digit, STDOUT);
    sprintf(check2, "c %d %d %d\n", col, digit, STDOUT);

    int response1, response2;
    send(C1_FD, check1);
    send(C2_FD, check2);

    scanf("%d", &response1);
    scanf("%d", &response2);
    conflict |= response1;
    conflict |= response2;
    return conflict;
}

int check_block(int digit)
{
    int conflict = 0;
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            if (B[i][j] == 0) continue;
            if (B[i][j] == digit) conflict = 1;
        }
    }
    return conflict;
}

void _handle_p_()
{
    int cell, digit;
    scanf("%d %d", &cell, &digit);
    int row = cell / 3;
    int col = cell % 3;
    int ERR = 0;

    if (solution_mode)              ERR = 1;
    else if (A[row][col] != 0)      ERR = 2;
    else if (check_block(digit))    ERR = 3;
    else if (check_row(row, digit)) ERR = 4;
    else if (check_col(col, digit)) ERR = 5;

    switch (ERR)
    {
        case 0: B[row][col] = digit; break;
        case 1: printf(RED_BOLD "ERROR: Solution mode ON" RESET); break;
        case 2: printf(RED_BOLD "ERROR: Read-only cell"   RESET); break;
        case 3: printf(RED_BOLD "ERROR: Block conflict"   RESET); break;
        case 4: printf(RED_BOLD "ERROR: Row conflict"     RESET); break;
        case 5: printf(RED_BOLD "ERROR: Column conflict"  RESET); break;
    }

    fflush(stdout);
    if (ERR != 0) sleep(2);
    draw_block();
}

void _handle_n_()
{
    solution_mode = 0;
    for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++)
    {
        scanf("%d", &A[i][j]);
        B[i][j] = A[i][j]; 
    }
    draw_block();
}


void _handle_r_()
{
    int row, digit, response_fd;
    scanf("%d %d", &row, &digit);
    scanf("%d", &response_fd);

    char conflict[10] = "0 \n";
    for (int j = 0; j < 3; j++) if (B[row][j] == digit) conflict[0] = '1';
    send(response_fd, conflict);
}

void _handle_c_()
{
    int col, digit, response_fd;
    scanf("%d %d", &col, &digit);
    scanf("%d", &response_fd);

    char conflict[10] = "0 \n";
    for (int i = 0; i < 3; i++) if (B[i][col] == digit) conflict[0] = '1';
    send(response_fd, conflict);
}

void _handle_q_()
{
    printf("Bye...");
    fflush(stdout);
    sleep(2);
    exit(0);
}

int main(int argc, char *argv[])
{
    BLK_NO = atoi(argv[1]);
    STDIN  = atoi(argv[2]);
    STDOUT = atoi(argv[3]);
    R1_FD = atoi(argv[4]);
    R2_FD = atoi(argv[5]);
    C1_FD = atoi(argv[6]);
    C2_FD = atoi(argv[7]);

    printf("\033[?25l"); // Hide cursor
    printf("Block %d ready...\n", BLK_NO);
    dup2(STDIN, STDIN_FILENO);

    char cmd;
    while (1)
    {
        if (scanf(" %c", &cmd) != 1) continue;
        switch (cmd)
        {
            case 'n': _handle_n_(); break;
            case 'p': _handle_p_(); break;
            case 'r': _handle_r_(); break;
            case 'c': _handle_c_(); break;
            case 'q': _handle_q_(); break;
            default:  printf(RED_BOLD "Invalid command: %c\n" RESET, cmd);
        }
    }
    return 0;
}
