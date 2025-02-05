#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

/*
=========================
dup2() system call in C:
-------------------------
- It is used to duplicate an existing file descriptor to a specified file descriptor.
- Thus creating a copy that refers to the same open file or resource.
- It returns the new file descriptor, or -1 if an error occurs.
- The new file descriptor is specified by the user and can be the same as the old file descriptor.

- Syntax: int dup2(int oldfd, int newfd);

*/

int main()
{
    int file_fd = open("output.out", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (file_fd < 0)
    {
        perror("open");
        return 1;
    }

    dup2(file_fd, STDOUT_FILENO);
    printf("This will be written to output.txt instead of the terminal!\n");
    close(file_fd);
    return 0;
}
