/*
Condition Variables in POSIX Threads (pthread)

-----------------------------------------------
1. Creating/Destroying a Condition Variable:
-----------------------------------------------
- pthread_cond_init() - Initializes a condition variable.
- pthread_cond_t cond = PTHREAD_COND_INITIALIZER; - Static initializer for condition variables.
- pthread_cond_destroy() - Destroys a condition variable when no longer needed.

-----------------------------------------------
2. Waiting on a Condition:
-----------------------------------------------
- pthread_cond_wait() - Causes a thread to wait on a condition variable.
   - Must be called only after locking a mutex.
   - Unlocks the mutex while waiting and locks it again once awakened.

- pthread_cond_timedwait() - Same as pthread_cond_wait() but with a timeout.

-----------------------------------------------
3. Waking Threads Based on Conditions:
-----------------------------------------------
- pthread_cond_signal() - Wakes one thread waiting on the specified condition.
- pthread_cond_broadcast() - Wakes all threads waiting on the specified condition.

-----------------------------------------------
4. Important Notes:
-----------------------------------------------
- Always pair a condition variable with a mutex to avoid race conditions.
- Ensure proper locking before calling pthread_cond_wait() to avoid deadlocks.
- Use pthread_cond_broadcast() if multiple threads are waiting on the condition.
- Always check the condition in a loop after pthread_cond_wait() to avoid spurious wakeups.
*/


#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

/*
Condition Variables are used when a thread must wait for a condition to become true before proceeding.

🔹 Condition Variable Functions;

    pthread_cond_init() — Initializes the condition variable.
    pthread_cond_wait() — Waits until a condition is signaled.
    pthread_cond_signal() — Wakes one waiting thread.
    pthread_cond_broadcast() — Wakes all waiting threads.
    pthread_cond_destroy() — Destroys the condition variable.

🔹 Usage:
   - pthread_cond_init(&condition_cond, NULL);              // Initialize the condition variable
   - pthread_cond_wait(&condition_cond, &condition_mutex);  // Wait for the condition
   - pthread_cond_signal(&condition_cond);                  // Signal one waiting thread
   - pthread_cond_broadcast(&condition_cond);               // Signal all waiting threads
   - pthread_cond_destroy(&condition_cond);                 // Destroy the condition variable
*/

pthread_mutex_t count_mutex     = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t condition_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  condition_cond  = PTHREAD_COND_INITIALIZER;

void *f1();
void *f2();
int  count = 0;

/*
- Note that f1() was halted while count was between the values COUNT_HALT1 and COUNT_HALT2.
- The only thing that has been ensures is that f2 will increment the count between the values COUNT_HALT1 and COUNT_HALT2.
- Everything else is random.
*/

#define COUNT_DONE  10
#define COUNT_HALT1  3
#define COUNT_HALT2  6

int main()
{
   pthread_t thread1, thread2;

   pthread_create( &thread1, NULL, &f1, NULL);
   pthread_create( &thread2, NULL, &f2, NULL);
   pthread_join( thread1, NULL);
   pthread_join( thread2, NULL);

   exit(0);
}

void *f1()
{
   while(1)
   {
      pthread_mutex_lock( &condition_mutex );
      while( count >= COUNT_HALT1 && count <= COUNT_HALT2 ) pthread_cond_wait( &condition_cond, &condition_mutex );
      pthread_mutex_unlock( &condition_mutex );

      pthread_mutex_lock( &count_mutex );
      count++;
      printf("Counter value f1: %d\n",count);
      pthread_mutex_unlock( &count_mutex );

      if(count >= COUNT_DONE) return(NULL);
    }
}

void *f2()
{
    while(1)
    {
       pthread_mutex_lock( &condition_mutex );
       if( count < COUNT_HALT1 || count > COUNT_HALT2 ) pthread_cond_signal( &condition_cond );
       pthread_mutex_unlock( &condition_mutex );

       pthread_mutex_lock( &count_mutex );
       count++;
       printf("Counter value f2: %d\n",count);
       pthread_mutex_unlock( &count_mutex );
       if(count >= COUNT_DONE) return(NULL);
    }

}