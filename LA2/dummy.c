#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main()
{
    signal(SIGINT, SIG_DFL);
    while (1) pause(); 
    return 0; 
}
