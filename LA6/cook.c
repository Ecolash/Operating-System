#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

#define SHM_SIZE 	2000
#define MIN_SCALE   100000

#define K0 ftok("makefile", 'M')
#define K1 ftok("makefile", 'K')
#define K2 ftok("makefile", 'C')
#define K3 ftok("makefile", 'W')
#define K4 ftok("makefile", 'U')

#define semwait(n, p) semop(n, &p, 1)
#define semsignal(n, p) semop(n, &p, 1)

void P(int semid, int posn)
{
	struct sembuf pop = {posn, -1, 0};
	int check = semwait(semid, pop);
	if (check == -1)
	{
		perror("P() - wait failed!");
		exit(EXIT_FAILURE);
	}
}

void V(int semid, int posn)
{
	struct sembuf vop = {posn, 1, 0};
	int check = semsignal(semid, vop);
	if (check == -1)
	{
		perror("V() - signal failed!");
		exit(EXIT_FAILURE);
	}
}

char *time_str(int time) 
{
	int hour = (time / 60) + 11;
	int minute = time % 60;
	char *str = (char *)malloc(20);
	if (!str) return NULL;

	char period = (hour >= 12) ? 'p' : 'a';
	hour = (hour % 12) ? (hour % 12) : 12;
	snprintf(str, 20, "[%02d:%02d %cm]", hour, minute, period);
	return str;
}

void cmain(){
	key_t shm_key = K0;
	key_t mtx_M_key = K1;
	key_t mtx_cook_key = K2;
	key_t mtx_waiter_key = K3;

	int shmid = shmget(shm_key, SHM_SIZE, 0666);
	int mtx_M = semget(mtx_M_key, 1, 0666);
	int mtx_cook = semget(mtx_cook_key, 1, 0666);
	int mtx_waiter = semget(mtx_waiter_key, 5, 0666);

	int *M = (int *)shmat(shmid, NULL, 0);
	if (M == (int *)-1) { perror("shmat failed"); exit(EXIT_FAILURE); }


	P(mtx_M, 0);
	int time = M[0];
	pid_t pid = getpid();
	char cook = (M[4] == pid) ? 'C' : 'D';
	V(mtx_M, 0);

	char *str = time_str(time);
	printf("%s ", str);
	if (cook == 'D') printf("  ");
	printf("Cook %c started\n", cook);
	fflush(stdout);
	free(str);

	while (1)
	{
		P(mtx_cook, 0);
		P(mtx_M, 0);
		
		int front = M[1100], rear = M[1101];
		char waiter = M[rear] + 'U';
		int customer_id = M[rear + 1];
		int customer_cnt = M[rear + 2];
		M[1101] = rear + 3;
		time = M[0];

		if (rear > front && time > 240)
		{
			printf("%s", time_str(time));
			if (cook == 'D') printf("  ");
			printf(" Cook %c: Leaving\n", cook);
			V(mtx_M, 0);
			break;
		}
		
		V(mtx_M, 0);
		char *str = time_str(time);
        printf("%s", str);
        if (cook == 'D') printf("  ");
        printf(" Cook %c: Preparing order (Waiter %c, Customer %d, Count %d)\n", cook, waiter, customer_id, customer_cnt);
        fflush(stdout);
        free(str);

        int delT = 5 * customer_cnt;
        usleep(delT * MIN_SCALE);

		P(mtx_M, 0);
		M[0] = time + delT;
		time = M[0];

		int waiter_id = waiter - 'U';
		int waiter_idx = 200 * waiter_id;
		M[waiter_idx + 100] = customer_id;
		front = M[1100];
		rear = M[1101];

		V(mtx_M, 0);
		V(mtx_waiter, waiter_id);

		char *str2 = time_str(time);
		printf("%s", str2);
		if (cook == 'D') printf("  ");
		printf(" Cook %c: Prepared order (Waiter %c, Customer %d, Count %d)\n", cook, waiter, customer_id, customer_cnt);
		fflush(stdout);
		free(str2);

		int f1 = (rear > front)? 1 : 0;
		int f2 = (time > 240)? 1 : 0;
		if (f1 + f2 == 2) 
		{
			char *str3 = time_str(time);
			printf("%s", str3);
			if (cook == 'D') printf("  ");
			printf(" Cook %c: Leaving\n", cook);
			fflush(stdout);
			free(str3);
			break;
		}
  }
  shmdt(M);
  exit(0);
}

int main(){
	key_t shm_key = K0;
	key_t mtx_M_key = K1;
	key_t mtx_cook_key = K2;
	key_t mtx_waiter_key = K3;
	key_t mtx_customer_key = K4;

	if (shm_key == -1 || mtx_M_key == -1 || mtx_cook_key == -1 || mtx_waiter_key == -1 || mtx_customer_key == -1) {
		perror("ftok failed");
		exit(EXIT_FAILURE);
	}

	int shmid = shmget(shm_key, SHM_SIZE * sizeof(int), IPC_CREAT | 0666);
	int mtx_M = semget(mtx_M_key, 1, IPC_CREAT | 0666);
	int mtx_cook = semget(mtx_cook_key, 1, IPC_CREAT | 0666);
	int mtx_waiter = semget(mtx_waiter_key, 5, IPC_CREAT | 0666);
	int mtx_customer = semget(mtx_customer_key, 200, IPC_CREAT | 0666);

	if (shmid == -1 ) {
		perror("shmget failed");
		exit(EXIT_FAILURE);
	}

	if (mtx_M == -1 || mtx_cook == -1 || mtx_waiter == -1 || mtx_customer == -1) {
		perror("semget failed");
		exit(EXIT_FAILURE);
	}

	int *M = (int *)shmat(shmid, NULL, 0);
	if (M == (int *)-1) {
		perror("shmat failed");
		exit(EXIT_FAILURE);
	}
	memset(M, 0, SHM_SIZE * sizeof(int));
	M[1] = 10;

	semctl(mtx_M, 0, SETVAL, 1);
    semctl(mtx_cook, 0, SETVAL, 0);
    for (int i = 0; i < 5; i++) semctl(mtx_waiter, i, SETVAL, 0);
    for (int i = 0; i < 200; i++) semctl(mtx_customer, i, SETVAL, 0);

    int cooks = 2;
    pid_t cpids[cooks];
	for (int i = 0; i < cooks; i++)
	{
		pid_t pid = fork();
		if (pid == 0)
		{
			cmain();
			exit(0);
		}
		else
		{
			P(mtx_M, 0);
			M[4 + i] = pid;
			cpids[i] = pid;
			V(mtx_M, 0);
		}
	}
	for (int i = 0; i < cooks; i++) waitpid(cpids[i], NULL, 0);
	for (int i = 0; i < 5; i++) V(mtx_waiter, i);
	shmdt(M);
	exit(0);
}


