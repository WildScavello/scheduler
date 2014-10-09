#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <math.h>
#include <stdarg.h>
#include <limits.h>

#define MAX_PRIORITY 5
//#define CTRL_WHITE "\033[0;37;40m"
#define CTRL_WHITE "\033[00m"
#define CTRL_YELLOW "\033[33m"
// DEBUG_LEVEL is a bitfield. It will mask events corresponding to the level.
// If you wish to mask all debug prints, set it to 0.
//#define DEBUG_LEVEL 0xffffffff
#define DEBUG_LEVEL 0

struct node {
	int tid;
	int remainingTime;
	float currentTime;
	int tprio;
	pthread_cond_t sleep;
	struct node* next;
};

struct node* Ready[MAX_PRIORITY];
struct node* Executing;
int (*Scheduler)(float, int, int, int);
float CurrentTime = -1.0f;
pthread_mutex_t SchedMutex;

static void debug(int level, const char *fmt, ...) {
	if (!(level & DEBUG_LEVEL))
		return;
	va_list args;
	va_start(args, fmt);

	// Provide color coding.
	fprintf(stderr, "\033[%dm", 31 + (int) log2(level));
	vfprintf(stderr, fmt, args);
	fprintf(stderr, CTRL_WHITE);
	va_end(args);
}

static void print_ready_queues() {
	debug(1, "Ready Queues:\n");
	for (int i = 0; i < MAX_PRIORITY; i++) {
		debug(1, "   Priority: %d\n", i + 1);
		struct node* head = Ready[i];
		while (head != NULL) {
			debug(1, "      tid: %d; currentTime: %f; remainingTime: %d\n", head->tid, head->currentTime, head->remainingTime);
			head = head -> next;
		}
	}
}

static void addQueueCurrentTimeIncreasing (struct node* toAdd) {
	struct node* head = Ready[toAdd->tprio];
	struct node* prev = head;
	toAdd->next = NULL;

	if (head == NULL) {
		Ready[toAdd->tprio] = toAdd;
		return;
	}
	while (head != NULL && head->currentTime < toAdd->currentTime) {
		prev = head;
		head = head->next;
	}
	if (prev == head) {
		toAdd->next = head;
		Ready[toAdd->tprio] = toAdd;
	} else {
		toAdd->next = head;
		prev->next = toAdd;
	}
}

static void popQueue (int tprio) {
	struct node* head = Ready[tprio];

	if(head == NULL)
		return;

	Ready[tprio] = head->next;
	free(head);
}

static void removeFromQueue (struct node* toRemove) {
	struct node* head = Ready[toRemove->tprio], *prev = head;

	if(head == NULL)
		return;

	while (head != NULL && head != toRemove) {
		prev = head;
		head = head->next;
	}
	if (prev == head) {
		if (head) {
			Ready[toRemove->tprio] = head->next;
		} else {
			Ready[toRemove->tprio] = NULL;
		}
	} else {
		if (head) {
			prev->next = head->next;
		}
	}
	free(toRemove);
}

static void bubbleSortCurrentTime(int queueIndex) {
	struct node* head = Ready[queueIndex], *prev = head, *next = head->next;
	while (next != NULL) {
		if (head->currentTime > next->currentTime) {
			if (prev == head) {
				// Current order is head -> next
				// Should be next -> head.
				head->next = next->next;
				next->next = head;
				Ready[queueIndex] = next;
			} else {
				// Current order is prev -> head -> next
				// Should be prev -> next -> head.
				head->next = next->next;
				next->next = head;
				prev->next = next;
			}
		} else {
			break;
		}
		prev = head;
		head = next;
		next = head -> next;
	}
}

int fcfs(float currentTime, int tid, int remainingTime, int tprio) {
	// Acquire mutex.
	pthread_mutex_lock(&SchedMutex);

	//fprintf(stderr, "t: %f; tid: %d; remainingTime: %d", currentTime, tid, remainingTime);

	// Identify if this is the first time the thread is requesting to be scheduled.
	// If it does not match with the thread "Executing", then it is.
	struct node* thread = NULL;
	if (Executing == NULL || tid != Executing->tid) {
		//fprintf(stderr, " (first arrival)\n");
		// Create a 'struct node' value to represent this thread.
		thread = malloc(sizeof(struct node));
		thread->remainingTime = remainingTime;
		thread->tid = tid;
		thread->currentTime = currentTime;
		thread->tprio = 0;
		pthread_cond_init(&thread->sleep, NULL);
		addQueueCurrentTimeIncreasing(thread);
	} else {
		//fprintf(stderr, "\n");
		thread = Executing;
		thread->remainingTime = remainingTime;
	}

	// If the thread executing should not be (that is, its 'currentTime' value
	// is greater than some other's), then signal to the next one and enter the
	// sleeping state.
	if (Ready[0]->tid != tid) {
		pthread_cond_signal(&Ready[0]->sleep);
		pthread_cond_wait(&thread->sleep, &SchedMutex);
	}

	// If this thread is done executing, remove it from the mix.
	if (thread->remainingTime == 0) {
		popQueue(0);
	} else {
		CurrentTime++;
		// If the thread is not ready to run, 'yield' for some amount of time.
		// For instance, the CurrentTime could be 12, yet the arrival time is
		// 12.1. As we're assuming non-divisible time slots, we should round up
		// to 13.
		if (CurrentTime < thread->currentTime) {
			CurrentTime += ceil(-CurrentTime + thread->currentTime);
		}
	}
	Executing = thread;
	pthread_mutex_unlock(&SchedMutex);

	// If this thread is done executing, send in a signal to the next thread in line.
	if (remainingTime == 0) {
		if (Ready[0] != NULL)
			pthread_cond_signal(&Ready[0]->sleep);
	}

	return CurrentTime;
}

int srtf(float currentTime, int tid, int remainingTime, int tprio) {

}

int pbs(float currentTime, int tid, int remainingTime, int tprio) {
	pthread_mutex_lock(&SchedMutex);

	debug(8, "gaining mutex: %d\n", tid);
	debug(4, "t: %f; tid: %d; remainingTime: %d", currentTime, tid, remainingTime);
	struct node* thread = NULL;
	if (Executing==NULL || Executing->tid != tid) {
		// Create a 'struct node' value to represent this thread.
		thread = malloc(sizeof(struct node));
		thread->remainingTime = remainingTime;
		thread->tid = tid;
		thread->currentTime = currentTime;
		thread->tprio = tprio - 1;
		pthread_cond_init(&thread->sleep, NULL);
		addQueueCurrentTimeIncreasing(thread);
		debug(4, " (first arrival)\n");
	} else {
		thread = Executing;
		thread->remainingTime = remainingTime;
		thread->currentTime = CurrentTime + 1;
		bubbleSortCurrentTime(thread->tprio);
		debug(4, "\n");
	}

	if (remainingTime == 0) {
		Executing = NULL;
		removeFromQueue(thread);
	}
	print_ready_queues();
	// Check to see if the current thread is the one that should be running. If
	// not, send a signal to the one that should be running, and go to sleep.
	int flag = 0;
	do {
		debug(16, "CurrentTime: %f\n", CurrentTime);
		flag = 0;
		for (int i = 0; i < MAX_PRIORITY; i++) {
			if (Ready[i] != NULL && Ready[i]->currentTime <= CurrentTime + 1) {
				if (Ready[i] != thread || (Executing != thread && Executing != NULL)) { // wait if another thread is already running or this thread is not the next in line.
					pthread_cond_signal(&Ready[i]->sleep);
					if (remainingTime == 0) {
						debug(8, "releasing mutex (totally done): %d\n", tid);
						pthread_mutex_unlock(&SchedMutex);
						return 0;
					}
					debug(8, "releasing mutex (yielding): %d\n", tid);
					pthread_cond_wait(&thread->sleep, &SchedMutex);
					debug(8, "gaining mutex (cond signal): %d\n", tid);
				}
				flag = 1;
				break;
			}
		}
		if (flag == 0) {
			debug(2, "%p %p %p %p %p\n", Ready[0], Ready[1], Ready[2], Ready[3], Ready[4]);
			int i;
			for (i = 0; i < MAX_PRIORITY; i++) {
				if (Ready[i] != NULL) {
					break;
				}
			}
			if (i == 5) {
				debug(2, "All threads finished.\n");
				break;
			}
			debug(2, "Threads left to finish. Current Time: %f\n", CurrentTime);
			debug(16, "CurrentTime: %f\n", CurrentTime);
			print_ready_queues();
		} else {
			debug(2, "broke out of PBS loop.\n");
//			print_ready_queues();
			break;
		}
		// If the code reaches this point, then there are still threads left to
		// execute, but their starting time is after CurrentTime. Increment
		// CurrentTime by the best value we can, then repeat.
		flag = INT_MAX;
		for (int i = 0; i < MAX_PRIORITY; i++) {
			if (Ready[i] != NULL && Ready[i]->currentTime - CurrentTime - 1 < flag) {
				flag = (int) ceil(Ready[i]->currentTime - CurrentTime - 1.0f);
			}
		}
		debug(2, "Incrementing CurrentTime by: %d\n", flag);
		CurrentTime += (float) flag;
		debug(16, "CurrentTime: %f\n", CurrentTime);
		flag = 0;
	} while (!flag);
	Executing = thread;

	CurrentTime++;
	debug(16, "CurrentTime: %f\n", CurrentTime);
	debug(8, "releasing mutex (finished): %d\n", tid);
	pthread_mutex_unlock(&SchedMutex);

	return CurrentTime;
}

int mlfq(float currentTime, int tid, int remainingTime, int tprio) {

}


void init_scheduler(int schedType) {
	switch(schedType) {
		case 0:
			Scheduler = &fcfs;
			break;
		case 1:
			Scheduler = &srtf;
			break;
		case 2:
			Scheduler = &pbs;
			break;
		case 3:
			Scheduler = &mlfq;
			break;
		default:
			fprintf(stderr, "unknown schedule type.");
	}
	pthread_mutex_init(&SchedMutex, NULL);
}

int scheduleme(float currentTime, int tid, int remainingTime, int tprio) {
	return (*Scheduler)(currentTime, tid, remainingTime, tprio);
}

