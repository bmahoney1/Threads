#include "ec440threads.h"    

#define MAX_NUM_THREADS 128

#define THREAD_CNT 3

#define QUANTUM 50

#include <stdio.h> 
#include <stdlib.h>
//#include <pthread.h>

void *count(void *arg) {
        unsigned long int c = (unsigned long int)arg;
        int i;
  
      for (i = 0; i < c; i++) {
/*
		printf("tid: 0x%x Just counted to %d of %ld\n", \
		(unsigned int)pthread_self(), i, c);
*/
		
                if ((i % 1000) == 0) {
//			printf("hello\n");
                        printf("tid: 0x%x Just counted to %d of %ld\n", \
                        (unsigned int)pthread_self(), i, c);
                }

        }
    return arg;
}

// waste some time
int main(int argc, char **argv) {
	pthread_t threads[THREAD_CNT];
	int i;
        unsigned long int cnt = 10000000;
        //unsigned long int cnt = 10000;

 //create THRAD_CNT threads
        for(i = 0; i<THREAD_CNT; i++) {
                pthread_create(&threads[i], NULL, count, (void *)((i+1)*cnt));
        }

    //join all threads ... not important for proj2
        //for(i = 0; i<THREAD_CNT; i++) {
        //      pthread_join(threads[i], NULL);
        //}
    // But we have to make sure that main does not return before 
    // the threads are done... so count some more...
    count((void *)(cnt*(THREAD_CNT + 1)));
//	while(1);
    return 0;
}

