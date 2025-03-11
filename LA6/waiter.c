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

#define K0	ftok("makefile", 'M')
#define K1  ftok("makefile", 'K')
#define K2  ftok("makefile", 'C')
#define K3  ftok("makefile", 'W')
#define K4  ftok("makefile", 'U')

#define semwait(n, p)  semop(n, &p, 1)
#define semsignal(n, p)  semop(n, &p, 1)

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

void wmain(){
	key_t shm_key = K0;
	key_t mtx_M_key = K1;
	key_t mtx_cook_key = K2;
	key_t mtx_waiter_key = K3;
	key_t mtx_customer_key = K4;

	int shmid = shmget(shm_key, SHM_SIZE, 0666);
	int mtx_M = semget(mtx_M_key, 1, 0666);
	int mtx_cook = semget(mtx_cook_key, 1, 0666);
	int mtx_waiter = semget(mtx_waiter_key, 5, 0666);
	int mtx_customer = semget(mtx_customer_key, 200, 0666);

	int *M = (int *)shmat(shmid, NULL, 0);
	if (M == (int *)-1) { perror("shmat failed"); exit(EXIT_FAILURE); }

	P(mtx_M, 0);
	pid_t pid = getpid();
	char waiter;
	int waiter_id;
	for (int i = 0; i < 5; i++) {
		if (pid == M[6 + i]) {
			waiter = 'U' + i;
			waiter_id = i;
			break;
		}
	}
	int time = M[0];
	int start = 200 * waiter_id;
	int waiting_orders = 0;
	V(mtx_M, 0);

	printf("%s %*cWaiter %c is ready\n", time_str(time), (waiter_id << 1) + 1, ' ', waiter);
	fflush(stdout);

    while(1){
		P(mtx_waiter, waiter_id);
		P(mtx_M, 0);
		time = M[0];
		int rear = M[start + 103];
		int front = M[start + 102];

		if (M[start + 100] != 0)
		{
			int customer_id = M[start + 100];
			printf("%s %*cWaiter %c: Serving food to Customer %d\n",  time_str(time), (waiter_id << 1) + 1, ' ', waiter, customer_id);
			waiting_orders--;
			fflush(stdout);

			M[start + 100] = 0;
			V(mtx_customer, customer_id - 1);

			int f1 = (rear > front) ? 1 : 0;
			int f2 = (time > 240) ? 1 : 0;
			if (f1 + f2 == 2 && waiting_orders == 0)
			{
				printf("%s %*cWaiter %c: Leaving\n", time_str(time), (waiter_id << 1) + 1, ' ', waiter);
				fflush(stdout);
				V(mtx_M, 0);
				V(mtx_cook, 0);
				break;
			}
		}

		int f1 = ((rear > front) || (rear == 0)) ? 1 : 0;
		int f2 = (time > 240) ? 1 : 0;
		if (f1 + f2 == 2 && waiting_orders == 0)
		{
			printf("%s %*cWaiter %c: Leaving\n", time_str(time), (waiter_id << 1) + 1, ' ', waiter);
			fflush(stdout);
			V(mtx_M, 0);
			V(mtx_cook, 0);
			break;
		}

		int customer_id = M[rear];
		if (rear == 0) { V(mtx_M, 0); continue; }
		if (customer_id == 0) { V(mtx_M, 0); continue; }

		M[start + 103] += 2;
		int customer_cnt = M[rear + 1];
		time = M[0];
		V(mtx_M, 0);

		usleep(MIN_SCALE);

		V(mtx_customer, customer_id - 1);
		P(mtx_M, 0);
		M[0] = time + 1;
		time = M[0];

		front = M[1100];
		if (front > 0)
		{
			M[1100] = front + 3;
			front = M[1100];
		}
		else
		{
			front = 1102;
			M[1100] = 1102;
			M[1101] = 1102;
		}
		M[front] = waiter_id;
		M[front + 1] = customer_id;
		M[front + 2] = customer_cnt;

		V(mtx_M, 0);
		printf("%s %*cWaiter %c: Placing order for Customer %d (count = %d)\n", time_str(time), (waiter_id << 1) + 1, ' ', waiter, customer_id, customer_cnt);
		fflush(stdout);
		waiting_orders++;
		V(mtx_cook, 0);
	}
	shmdt(M);
	exit(0);
}

int main(){
	key_t shm_key = K0;
	key_t mtx_M_key = K1;

	int shmid = shmget(shm_key, SHM_SIZE, 0666);
	int mtx_M = semget(mtx_M_key, 1, 0666);
	int *M = (int *)shmat(shmid, NULL, 0);

	if (M == (int *)-1)
	{
		perror("shmat failed");
		exit(EXIT_FAILURE);
	}

	pid_t wpids[5];
	for (int i = 0; i < 5; i++)
	{
		pid_t pid = fork();
		if (pid == 0) wmain();
		else
		{
			P(mtx_M, 0);
			wpids[i] = pid;
			M[6 + i] = pid;
			V(mtx_M, 0);
		}
	}
	for (int i = 0; i < 5; i++) waitpid(wpids[i], NULL, 0);
    shmdt(M);
    return 0;
}