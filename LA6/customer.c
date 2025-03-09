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

#define SHM_SIZE    2000
#define MIN_SCALE   100000
#define FILENAME    "customers.txt"

void P(int semid, int posn)
{
	struct sembuf pop = {posn, -1, 0};
	int check = semop(semid, &pop, 1);
	if (check == -1)
	{
		perror("P() - wait failed!");
		exit(EXIT_FAILURE);
	}
}

void V(int semid, int posn)
{
	struct sembuf vop = {posn, 1, 0};
	int check = semop(semid, &vop, 1);
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

void cmain(int customer_id, int T, int customer_cnt)
{
	key_t shm_key = ftok("makefile", 'M');
	key_t mtx_M_key = ftok("makefile", 'K');
	key_t mtx_waiter_key = ftok("makefile", 'W');
	key_t mtx_customer_key = ftok("makefile", 'U');

	int shmid = shmget(shm_key, SHM_SIZE, 0666);
	int mtx_M = semget(mtx_M_key, 1, 0666);
	int mtx_waiter = semget(mtx_waiter_key, 5, 0666);
	int mtx_customer = semget(mtx_customer_key, 200, 0666);

	int *M = (int *)shmat(shmid, NULL, 0);
	if (M == (int *)-1)
	{
		perror("shmat failed");
		exit(EXIT_FAILURE);
	}
	if (T > 240)
	{
		char *str = time_str(T);
		printf("%s", str);
		for(int i = 0; i < 5; i++) printf("  ");
		printf("Customer %d leaves (late arrival)\n", customer_id);
		free(str);
		shmdt(M);
		exit(0);
	}

	P(mtx_M, 0);
	M[0] = T;
	int table = M[1];
	V(mtx_M, 0);
	if (table == 0)
	{
		char *str = time_str(T);
		printf("%s", str);
		for(int i = 0; i < 5; i++) printf("  ");
		printf("Customer %d leaves (no empty table)\n", customer_id);
		free(str);
		shmdt(M);
		exit(0);
	}

	char *str = time_str(T);
	printf("%s", str);
	printf(" Customer %d arrives (count = %d)\n", customer_id, customer_cnt);
	free(str);

	P(mtx_M, 0);
	M[1]--;

	int waiter_id = M[2];
	char waiter = M[2] + 'U';
	char next = (M[2] + 1) % 5;
	M[2] = next;

	int start = 200 * waiter_id;
	int front = M[start + 102];
	if (front != 0)
	{
		front = front + 2;
		M[start + 102] = front;
	}
	else
	{
		front = start + 104;
		M[start + 102] = front;
		M[start + 103] = front;
	}
	M[front] = customer_id;
	M[front + 1] = customer_cnt;

	V(mtx_waiter, waiter_id);
	V(mtx_M, 0);

	P(mtx_customer, customer_id - 1);
	P(mtx_M, 0);
	int time = M[0];
	V(mtx_M, 0);

	str = time_str(time);
	printf("%s", str);
	printf("    Customer %d: Order placed to Waiter %c\n", customer_id, waiter);
	free(str);
	P(mtx_customer, customer_id - 1);

	P(mtx_M, 0);
	time = M[0];
	V(mtx_M, 0);
	str = time_str(time);
	printf("%s\t Customer %d gets food [Waiting time = %d]\n", str, customer_id, time - T);
	free(str);

	usleep(30 * MIN_SCALE);
	P(mtx_M, 0);
	M[0] = time + 30;
	M[1]++;
	V(mtx_M, 0);

	str = time_str(time + 30);
	printf("%s\t   Customer %d finishes eating and leaves\n", str, customer_id);
	free(str);
	shmdt(M);
	exit(0);
}

int main(){
	key_t shm_key = ftok("makefile", 'M');
	key_t mtx_M_key = ftok("makefile", 'K');
	key_t mtx_cook_key = ftok("makefile", 'C');
	key_t mtx_waiter_key = ftok("makefile", 'W');
	key_t mtx_customer_key = ftok("makefile", 'U');

	int shmid = shmget(shm_key, SHM_SIZE, 0666);
	int mtx_M = semget(mtx_M_key, 1, 0666);
	int mtx_cook = semget(mtx_cook_key, 1, 0666);
	int mtx_waiter = semget(mtx_waiter_key, 5, 0666);
	int mtx_customer = semget(mtx_customer_key, 200, 0666);

	int *M = (int *)shmat(shmid, NULL, 0);
	if (M == (int *)-1)
	{
		perror("shmat failed");
		exit(EXIT_FAILURE);
	}

  	FILE* fp = fopen(FILENAME, "r");
  	if (fp == NULL) { perror("fopen"); fclose(fp); exit(EXIT_FAILURE); }

	int last = 0;
	P(mtx_M, 0);
	int customers = 0;
	pid_t cpids[1000];

	while (1)
	{
		int time = 0;
		int customer_id = 0; 
		int customer_cnt = 0;
		if (fscanf(fp, "%d %d %d", &customer_id, &time, &customer_cnt) != 3)
		{
			V(mtx_M, 0);
			break; 
		}
		int delT = time - last;
		if (delT > 0)
		{
			V(mtx_M, 0);
			usleep(delT * MIN_SCALE);
			P(mtx_M, 0);
		}
		if (customer_id == -1)
		{
			V(mtx_M, 0);
			break;
		}
		pid_t pid = fork();
		if (pid == 0) cmain(customer_id, time, customer_cnt);
		else
		{
			customers++;
			cpids[customers - 1] = pid;
			last = time;
		}
	}
	V(mtx_M, 0);
	for (int i = 0; i < customers; i++) waitpid(cpids[i], NULL, 0);

	fclose(fp);
 	int c0 = shmctl(shmid, IPC_RMID, NULL);
    int c1 = semctl(mtx_M, 0, IPC_RMID, 0);
    int c2 = semctl(mtx_cook, 0, IPC_RMID, 0);
    int c3 = semctl(mtx_waiter, 0, IPC_RMID, 0);
    int c4 = semctl(mtx_customer, 0, IPC_RMID, 0);

    if (c0 == -1) perror("shmctl");
    if (c1 == -1) perror("semctl for M");
    if (c2 == -1) perror("semctl for cook");
    if (c3 == -1) perror("semctl for waiter");
    if (c4 == -1) perror("semctl for customer");
    shmdt(M);
	return 0;
}
