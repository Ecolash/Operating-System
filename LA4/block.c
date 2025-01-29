#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        fprintf(stderr, "Usage: %s <block_no> <fd_in> <fd_out>\n", argv[0]);
        exit(1);
    }

    int block_no = atoi(argv[1]);
    int fd_in = atoi(argv[2]);
    int fd_out = atoi(argv[3]);

    printf("Block %d ready...\n", block_no);
    fflush(stdout);

    while (1)
    {
        char buffer[256];
        if (read(fd_in, buffer, sizeof(buffer)) > 0)
        {
            printf("Block %d received: %s\n", block_no, buffer);
            fflush(stdout);
        }
    }

    return 0;
}
