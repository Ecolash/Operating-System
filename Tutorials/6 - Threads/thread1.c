/*
Thread Management in C using pthreads

1) pthread_create
   - Function prototype:
     int pthread_create(pthread_t *thread,const pthread_attr_t *attr,void *(*start_routine)(void *),void *arg);

   - Arguments:
     - `thread`: Returns the thread ID (unsigned long int defined in `bits/pthreadtypes.h`).
     - `attr`: Thread attributes (set to NULL for default attributes).
     - `start_routine`: Pointer to the function executed by the thread. The function must take a `void*` argument and return a `void*`.
     - `arg`: Pointer to the argument for the thread function. For multiple arguments, pass a pointer to a structure.

   - Return value: 0 on success, or an error number on failure.

2) pthread_exit

   - Function prototype:  void pthread_exit(void *retval);
   - Description: Terminates the calling thread and returns a value to any joining thread.
   - Arguments: `retval`: Return value of the thread.

3) Thread Termination
   Threads can terminate in three ways:
   - By explicitly calling `pthread_exit`.
   - By letting the thread function return.
   - By calling `exit()`, which terminates the entire process (including all threads).
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

void *print_message_function(void *ptr);

int main()
{
    pthread_t thread1, thread2;
    char *message1 = "Thread 1";
    char *message2 = "Thread 2";
    int iret1, iret2;

    /* Create independent threads each of which will execute function (Return 0 if success) */

    iret1 = pthread_create(&thread1, NULL, print_message_function, (void *)message1);
    iret2 = pthread_create(&thread2, NULL, print_message_function, (void *)message2);

    /* Wait till threads are complete before main continues. */
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    

    printf("Thread 1 returns: %d\n", iret1);
    printf("Thread 2 returns: %d\n", iret2);
    exit(0);
}

void *print_message_function(void *ptr)
{
    char *message;
    message = (char *)ptr;
    printf("%s \n", message);
}