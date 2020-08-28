/* Author: Tess						 	    */
/* Listing 23-7: POSIX timer notification using a a thread function */
/*------------------------------------------------------------------*/

#include <time.h>
#include <signal.h>
#include <pthread.h>
#include "curr_time.h"			/* Declaration of currTime() */
#include "itimerspec_from_str.h"
#include "tlpi_hdr.h"			/* Declars itimerspecFromStr() */

static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static int expireCnt = 0;	/* Number of expirations of all timers */ 

/* Thread notification function */
static void threadFunc(union sigval sv)
{
	timer_t *tidptr;
	int s;
	
	tidptr = sv.sival_ptr;
	
	printf("[%s] Thread notify\n", currTime("%T"));
	printf("	timer ID = %ld\n", (long)*tidptr);
	printf("	timer_getoverrun() = %d\n", timer_getoverrun(*tidptr));

	/* Increment counter variable shared with main thread and signal 
	   condition variable to notify main thread of the change */
	
	s = pthread_mutex_lock(&mtx);
	if (s != 0)
		errExitEN(s, "pthread_mutex_lock");
	
	expireCnt += 1 + timer_getoverrun(*tidptr);
	
	s = pthread_mutex_unlock(&mtx);
	if (s != 0)
		errExitEN(s, "pthread_mutex_unlock");

	s = pthread_cond_signal(&cond);
	if (s != 0)
		errExitEN(s, "pthread_cond_signal");
}


int main(int argc, char* argv[])
{
	struct sigevent sev;
	struct itimerspec ts;
	timer_t *tidlist;
	int s, j;
	
	if (argc < 2)
		usageErr("%s secs[/nsecs][:int-secs[/int-nsecs]]...\n", argv[0]);
	
	tidlist = calloc(argc -1, sizeof(timer_t));
	if (tidlist == NULL)
		errExit("malloc");
	
	sev.sigev_notify = SIGEV_THREAD;		/* Notify via thread */
	sev.sigev_notify_function = threadFunc;		/* Thread start function */	
	sev.sigev_notify_attributes = NULL;	
	
	/* Create and start one timer for each command-line argument */
	for (j = 0; j < argc - 1; j++)
	{
		itimerspecFromStr(argv[j+1], &ts);
			
		sev.sigev_value.sival_ptr = &tidlist[j];
		
		if (timer_create(CLOCK_REALTIME, &sev, &tidlist[j]) == -1)
			errExit("timer_create");
		printf("Timer ID: %ld (%s)\n", (long)tidlist[j], argv[j+1]);
		
		if (timer_settime(tidlist[j], 0, &ts, NULL) == -1)
			errExit("timer_settime");
	}
	
	/* The main thread waits on a condition variable that is signaled 
	  on each invocation of the thread notification fucntion.
          We print a message so that the user can seee that this occurred. */
	
	s = pthread_mutex_lock(&mtx);
	if (s != 0)
		errExitEN(s, "pthread_mutex_lock");
	
	for (;;)
	{
		s = pthread_cond_wait(&cond, &mtx);
		if (s != 0)
			errExitEN(s, "pthread_cond_wait");
		printf("main(): expireCnt=%d\n", expireCnt);
	}
}
