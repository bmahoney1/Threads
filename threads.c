#include "ec440threads.h"

#include <setjmp.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
/* These are included with ec440threads.h
#include <stdio.h> 
#include <stdlib.h> 
#include <pthread.h>
*/

// Definitions //

#define MAX_NUM_THREADS 128


#define THREAD_STACK_SIZE 32767


#define QUANTUM 50


#define JB_RBX 0
#define JB_RBP 1
#define JB_R12 2
#define JB_R13 3
#define JB_R14 4
#define JB_R15 5
#define JB_RSP 6
#define JB_PC 7

// Thread Struct //

struct thread {
        int id;
        void *stack;
        jmp_buf regs;
        char status[30];

};

// Global structs/variables //

struct thread Thread_Table[MAX_NUM_THREADS];// This creates the thread table which will store all the threads and their corresponding info
pthread_t runningThreadID = 0;
struct sigaction signal_handler;
int first = 1;
int threadcount = 0;
// Functions //
unsigned long int ptr_demangle(unsigned long int p)
{
    unsigned long int ret;

    asm("movq %1, %%rax;\n"
        "rorq $0x11, %%rax;"
        "xorq %%fs:0x30, %%rax;"
        "movq %%rax, %0;"
    : "=r"(ret)
    : "r"(p)
    : "%rax"
    );
    return ret;
}

unsigned long int ptr_mangle(unsigned long int p)
{
    unsigned long int ret;

    asm("movq %1, %%rax;\n"
        "xorq %%fs:0x30, %%rax;"
        "rolq $0x11, %%rax;"
        "movq %%rax, %0;"
    : "=r"(ret)
    : "r"(p)
    : "%rax"
    );
    return ret;
}

void *start_thunk() {
  asm("popq %%rbp;\n"           //clean up the function prolog
      "movq %%r13, %%rdi;\n"    //put arg in $rdi
      "pushq %%r12;\n"          //push &start_routine
      "retq;\n"                 //return to &start_routine
      :
      :
      : "%rdi"
 );
        __builtin_unreachable();
}
void setup(){
	int i;
	
	for( i = 0; i < MAX_NUM_THREADS; i++){
		strcpy(Thread_Table[i].status, "EMPTY!");
		Thread_Table[i].id = i;
	}
	//      setjmp(Thread_Table[0].regs);   
//      runningThreadID++;
        
//      threadcount++;  
        // Setting up the ualarm to go off every 50 milliseconds, forcing a schedule
	
	useconds_t usecs = QUANTUM * 1000; // How long until the initial alarm is set off
	useconds_t interval = QUANTUM * 1000;// The intervals in which the alarm will continue to go off

	ualarm(usecs, interval);// Calls the ualarm function

	// Handling the sigalarm generated by ualarm
	sigemptyset(&signal_handler.sa_mask);
	signal_handler.sa_handler = &schedule;
	signal_handler.sa_flags = SA_NODEFER;	
	sigaction(SIGALRM, &signal_handler, NULL);
}

int pthread_create(
	pthread_t *thread, const pthread_attr_t *attr,
	void *(*start_routine) (void *), void *arg)
{
	
	attr = NULL;
	if (first){ // Checking to see if it is the first time pthread_create is being called
		setup(); // This sets up all the threads
		first = 0; // This makes it so that this never runs again (global)
		threadcount++;
		strcpy(Thread_Table[0].status,"READY!"); // This sets the main thread to ready
	}
	
		int i;
		for (i = 0; i< MAX_NUM_THREADS; i++){
        		if (strcmp(Thread_Table[i].status, "EMPTY!") == 0){
                		Thread_Table[i].id = i; // This sets the thread ID
                                *thread = i; // In the project description is says to set the *thread to the id #                 
                                Thread_Table[i].stack = malloc(THREAD_STACK_SIZE);
                                //printf("Thread id: %d was made!\n", Thread_Table[i].id);
                                break;
                        }
                        if (i == MAX_NUM_THREADS){
                                printf("Error: The maximum number of threads were created!\n");
                                exit(EXIT_FAILURE);
                        }
                }
	threadcount++;

      // setjmp(Thread_Table[i].regs);
        Thread_Table[i].regs[0].__jmpbuf[JB_PC] = ptr_mangle((unsigned long int)start_thunk);// This function moves arginto start_routine which should start the thread 
		
        Thread_Table[i].regs[0].__jmpbuf[JB_R13] = (long unsigned int) arg;// This is the argument that start_routine takes  

        Thread_Table[i].regs[0].__jmpbuf[JB_R12] = (unsigned long int) start_routine;// This is what simulates the thread is doing something
		

	 *(unsigned long int *)(Thread_Table[i].stack + THREAD_STACK_SIZE - 8) = (unsigned long int) pthread_exit;// Sets pthread_exit to the top of the stack, so when it returns to the top (finishes) it will exit

        Thread_Table[i].regs[0].__jmpbuf[JB_RSP] = ptr_mangle((unsigned long int)Thread_Table[i].stack + THREAD_STACK_SIZE -8);// This sets the stack pointer to the top of the thread

        strcpy(Thread_Table[i].status,"READY!");// Now that all the above is done, we can set the thread as Ready to be used
	
	return 0;
}

void pthread_exit(void *value_ptr){
	strcpy(Thread_Table[runningThreadID].status, "EXITED");
	
	threadcount--;
	if(threadcount != 0){
		schedule();
	}
	int i;
	for(i = 0; i < MAX_NUM_THREADS; i++){
		if(strcmp(Thread_Table[i].status,"EXITED") == 0){
			free(Thread_Table[i].stack);
		}
	}
	exit(0);
	__builtin_unreachable();
}

pthread_t pthread_self(void){
        return runningThreadID;
}

void schedule(){
	//printf("Hello: %d\n", threadcount);
        // schedule() variables // 
        //if (threadcount == 0){
        //      exit(0);

        //}

        if(strcmp(Thread_Table[runningThreadID].status, "EXITED") != 0){
                strcpy(Thread_Table[runningThreadID].status,"READY!");
        }

        pthread_t newID = runningThreadID;
        while(1){
                if(newID == MAX_NUM_THREADS - 1){
                        newID = 0;
                }
                else{
                        newID++;// Increment the id number
                }

                if(strcmp(Thread_Table[newID].status, "READY!") == 0){
                        break;
                }
        }

        int save = 0;// I use this variable to save the setjump location
        if(strcmp(Thread_Table[runningThreadID].status, "EXITED") != 0){
                save = setjmp(Thread_Table[runningThreadID].regs);

        }

	// When the same thread above is schedules and comes back to this point, save = 1 (the second argument in longjump) therefore will skip over it
        if(save != 1){
                runningThreadID = newID;
                strcpy(Thread_Table[runningThreadID].status, "RUNNIN");
                longjmp(Thread_Table[runningThreadID].regs, 1);
        }
}

