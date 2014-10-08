#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <math.h>

struct node {
	int tid;
	int remainingTime;
	float currentTime;
	int tprio;
	pthread_cond_t sleep;
	struct node* next;
};

struct node* Ready[5];
struct node* Executing;
int (*Scheduler)(float, int, int, int);
float CurrentTime = -1.0f;
pthread_cond_t WakeUp;
pthread_mutex_t SchedMutex;


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

static void popQueue (int tprio)
{
	struct node* head = Ready[tprio];

	if(head == NULL)
		return;

	Ready[tprio] = head->next;
	free(head);
}

int fcfs(float currentTime, int tid, int remainingTime, int tprio) {
	// Acquire mutex.
	pthread_mutex_lock(&SchedMutex);

//	fprintf(stderr, "t: %f; tid: %d; remainingTime: %d", currentTime, tid, remainingTime);

	// Identify if this is the first time the thread is requesting to be scheduled.
	// If it does not match with the thread "Executing", then it is.
	struct node* thread = NULL;
	if (Executing == NULL || tid != Executing->tid) {
//		fprintf(stderr, " (first arrival)\n");
		// Create a 'struct node' value to represent this thread.
		thread = malloc(sizeof(struct node));
		thread->remainingTime = remainingTime;
		thread->tid = tid;
		thread->currentTime = currentTime;
		thread->tprio = 0;
		pthread_cond_init(&thread->sleep, NULL);
		addQueueCurrentTimeIncreasing(thread);
	} else {
//		fprintf(stderr, "\n");
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

void init_scheduler(int schedType) {
	pthread_mutex_init(&SchedMutex, NULL);
	Scheduler = &fcfs;
}

int scheduleme(float currentTime, int tid, int remainingTime, int tprio) {
	return (*Scheduler)(currentTime, tid, remainingTime, tprio);
}

