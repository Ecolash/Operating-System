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

void semwait(int semid, int posn)
{
    struct sembuf pop = {posn, -1, 0};
    int check = semop(semid, &pop, 1);
    if (check == -1)
    {
        perror("semwait failed!");
        exit(EXIT_FAILURE);
    }
}

void semsignal(int semid, int posn)
{
    struct sembuf vop = {posn, 1, 0};
    int check = semop(semid, &vop, 1);
    if (check == -1)
    {
        perror("semsignal failed!");
        exit(EXIT_FAILURE);
    }
}

char *time_str(int time)
{
	int hour = (time / 60) + 11;
	int minute = time % 60;
	char *str = (char *)malloc(20 * sizeof(char));

	if (hour >= 12) sprintf(str, "[%02d:%02d pm]", (hour % 12) ? (hour % 12) : 12, minute);
	else sprintf(str, "[%02d:%02d am]", (hour % 12) ? (hour % 12) : 12, minute);
	return str;
}

void wmain(){
	key_t shm_key = ftok("makefile", 'M');
	key_t mtx_M_key = ftok("makefile", 'K');
	key_t mtx_cook_key = ftok("makefile", 'C');
	key_t mtx_waiter_key = ftok("makefile", 'W');
	key_t mtx_customer_key = ftok("makefile", 'U');

	int shmid = shmget(shm_key, 2000, 0666);
	int mtx_M = semget(mtx_M_key, 1, 0666);
	int mtx_cook = semget(mtx_cook_key, 1, 0666);
	int mtx_waiter = semget(mtx_waiter_key, 5, 0666);
	int mtx_customer = semget(mtx_customer_key, 200, 0666);

	int *M = (int *)shmat(shmid, NULL, 0);
	if (M == (int *)-1) { perror("shmat failed"); exit(EXIT_FAILURE); }

	semwait(mtx_M, 0);
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
	semsignal(mtx_M, 0);

	printf("%s ", time_str(time));
	for (int i = 0; i < waiter - 'U'; i++) printf("  ");
	printf("Waiter %c is ready\n", waiter);
	fflush(stdout);

    while(1){
		semwait(mtx_waiter, waiter_id);
		semwait(mtx_M, 0);
		time = M[0];
		int rear = M[start + 103];
		int front = M[start + 102];

		if (M[start + 100] != 0)
		{
			int customer_id = M[start + 100];
			printf("%s ", time_str(time));
			for (int i = 0; i < waiter - 'U'; i++) printf("  ");
			printf(" Waiter %c: Serving food to Customer %d\n", waiter, customer_id);
			fflush(stdout);

			M[start + 100] = 0;
			semsignal(mtx_customer, customer_id - 1);

			int f1 = (rear > front) ? 1 : 0;
			int f2 = (time > 240) ? 1 : 0;
			if (f1 + f2 == 2)
			{
				printf("%s ", time_str(time));
				for (int i = 0; i < waiter - 'U'; i++) printf("  ");
				printf(" Waiter %c: Leaving\n", waiter);
				fflush(stdout);
				semsignal(mtx_M, 0);
				break;
			}
		}

		int f1 = (rear > front) ? 1 : 0;
		int f2 = (time > 240) ? 1 : 0;
		if (f1 + f2 == 2)
		{
			printf("%s ", time_str(time));
			for (int i = 0; i < waiter - 'U'; i++) printf("  ");
			printf(" Waiter %c: Leaving\n", waiter);
			fflush(stdout);
			semsignal(mtx_M, 0);
			break;
		}

		int customer_id = M[rear];
		if (rear == 0) { semsignal(mtx_M, 0); continue; }
		if (customer_id == 0) { semsignal(mtx_M, 0); continue; }

		M[start + 103] += 2;
		int customer_cnt = M[rear + 1];
		time = M[0];
		semsignal(mtx_M, 0);

		usleep(MIN_SCALE);

		semsignal(mtx_customer, customer_id - 1);
		semwait(mtx_M, 0);
		M[0] = time + 1;
		time = M[0];

		front = M[1100];
		if (front > 0)
		{
			// Add the customer to the queue
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

		semsignal(mtx_M, 0);
		printf("%s ", time_str(time));
		for (int i = 0; i < waiter - 'U'; i++)printf("  ");
		printf(" Waiter %c: Placing order for Customer %d (count=%d)\n", waiter, customer_id, customer_cnt);
		semsignal(mtx_cook, 0);
	}
	shmdt(M);
	exit(0);
}

int main(){
	key_t shm_key = ftok("makefile", 'M');
	key_t mtx_M_key = ftok("makefile", 'K');

	int shmid = shmget(shm_key, 2000, 0666);
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
			semwait(mtx_M, 0);
			wpids[i] = pid;
			M[6 + i] = pid;
			semsignal(mtx_M, 0);
		}
	}
	for (int i = 0; i < 5; i++) waitpid(wpids[i], NULL, 0);
    shmdt(M);
    return 0;
}
