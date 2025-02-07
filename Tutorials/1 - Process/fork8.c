#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main()
{
    for(int i = 0; i < 3; i++)
    {
        printf("%d ", i + 1);
        // fflush(stdout);
        fork();
    }
    printf("\n");
    return 0;
}
