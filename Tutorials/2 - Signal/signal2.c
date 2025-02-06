#include <stdio.h>
#include <signal.h>
#include <unistd.h>

void signal_handler(int sig)
{
    printf("\nReceived signal %d: Exiting gracefully...\n", sig);
    exit(0);
}

int main()
{
    signal(SIGINT, signal_handler);
    printf("Running... Press Ctrl+C to send SIGINT\n");
    while (1) {}
    return 0;
}