#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

/*
=========================
dup() system call in C:
-------------------------
- It is used to duplicate an existing file descriptor.
- Thus creating a copy that refers to the same open file or resource.
- It returns a new file descriptor that refers to the same file as the original file descriptor.
- The new file descriptor is guaranteed to be the lowest-numbered file descriptor not currently open for the process.

- Syntax: int dup(int oldfd);

*/

int main()
{
    int file_fd = open("output.out", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (file_fd < 0) { perror("open"); return 1;}

    int fd1 = dup(file_fd);
    int fd2 = dup(STDOUT_FILENO);
    if (fd1 < 0) { perror("dup"); return 1; }
    if (fd2 < 0) { perror("dup"); return 1; }
    close(file_fd);
    
    printf("MSG 1: Output is in STDOUT\n");
    close(STDOUT_FILENO);
    dup(fd1);

    printf("MSG 2: Output is in file output.txt\n");
    close(STDOUT_FILENO);
    dup(fd2);

    printf("MSG 3: Output is in STDOUT\n");
    close(STDOUT_FILENO);
    dup(fd1);

    printf("MSG 4: Output is in file output.txt\n");
    close(fd1);
    close(fd2);

    return 0;
}
