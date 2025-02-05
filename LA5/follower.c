#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define MAX_FOLLOWERS   100 

int main(int argc, char *argv[])
{
    int *M;
    int shmid;
    int nf = (argc > 1) ? atoi(argv[1]) : 1;

    key_t key = ftok("/", 'T');
   
    for (int i = 0; i < nf; i++)
    {
        pid_t pid = fork();

        if (pid < 0)
        {
            perror("[-] fork failed");
            return EXIT_FAILURE;
        }
        else if (pid == 0)
        { 
            srand(time(NULL) + getpid());

            int fid;
            int shmid = shmget(key, 0, 0777);
            if (shmid == -1)
            {
                perror("[-] shmget failed");
                exit(EXIT_FAILURE);
            }

            M = (int *)shmat(shmid, NULL, 0);
            while (1)
            {
                int current_count = M[1];
                if (current_count < M[0])
                {
                    fid = current_count + 1;
                    M[1] = fid;
                    printf("[+] Follower %d joins\n", fid);
                    break;
                }
                else
                {
                    printf("[-] Follower limit reached. Exiting...\n");
                    exit(EXIT_SUCCESS);
                }
            }

            while (1)
            {
                while(M[2] != fid && M[2] != -fid) { };
                if (M[2] == fid)
                {
                    int x = rand() % 9 + 1;
                    M[3 + fid] = x;
                    M[2] = (fid == M[0]) ? 0 : fid + 1;
                }
                else if (M[2] == -fid)
                {
                    printf("[+] Follower %d leaves\n", fid);
                    M[2] = (fid == M[0]) ? 0 : -(fid + 1);
                    break;
                }
            }

            shmdt(M);
            exit(EXIT_SUCCESS);
        }
    }

    for (int i = 0; i < nf; i++) wait(NULL);
    return 0;
}
