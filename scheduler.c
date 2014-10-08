#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

struct node {
	int tid;
	int remainingTime;
	int currentTime;
	int tprio;
	pthread_cond_t sleep;
	struct node* next;
};

struct node* Ready[5];
struct node* Executing;
int (*Scheduler)(float, int, int, int);
float CurrentTime = 0.0f;
pthread_cond_t WakeUp;
pthread_mutex_t SchedMutex;


static void addQueueCurrentTimeIncreasing (struct node* toAdd) {
	struct node* head = Ready[toAdd->tprio];
	struct node* prev = head;
	toAdd->next = NULL;

	if (head == NULL) {
		Ready[toAdd->tprio] = toAdd;
		printf("\taddQueue (beginning): %d\n", toAdd->tid);
		return;
	}
	while (head != NULL && head->currentTime < toAdd->currentTime) {
		prev = head;
		head = head->next;
	}
	if (prev == head) {
		printf("\taddQueue (beginning, precedes %d): %d\n", head->tid, toAdd->tid);
		toAdd->next = head;
		Ready[toAdd->tprio] = toAdd;
	} else {
		printf("\taddQueue (succeeds %d, precedes %d): %d\n", prev->tid, head->tid, toAdd->tid);
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

	fprintf(stderr, "t: %f; tid: %d; remainingTime: %d", currentTime, tid, remainingTime);

	// Identify if this is the first time the thread is requesting to be scheduled.
	// If it does not match with the thread "Executing", then it is.
	struct node* thread = NULL;
	if (Executing == NULL || tid != Executing->tid) {
		fprintf(stderr, " (first arrival)\n");
		// Create a 'struct node' value to represent this thread.
		thread = malloc(sizeof(struct node));
		thread->remainingTime = remainingTime;
		thread->tid = tid;
		thread->currentTime = currentTime;
		thread->tprio = 0;
		pthread_cond_init(&thread->sleep, NULL);
		addQueueCurrentTimeIncreasing(thread);
	} else {
		fprintf(stderr, "\n");
		thread = Executing;
		thread->remainingTime = remainingTime;
	}

	// If the thread executing should not be (that is, its 'currentTime' value
	// is greater than some other's), then signal to the next one and enter the
	// sleeping state.
	if (Ready[0] != NULL)
		fprintf(stderr, "Arrived [%d] to find ready queue holding: %d\n", tid, Ready[0]->tid);
	else
		fprintf(stderr, "Arrived [%d] to find empty ready queue.\n", tid);
	if (Ready[0]->tid != tid) {
		pthread_cond_signal(&Ready[0]->sleep);
		pthread_cond_wait(&thread->sleep, &SchedMutex);
	}

	// If this thread is done executing, remove it from the mix.
	if (remainingTime == 0) {
		popQueue(0);
	}
	Executing = thread;
	pthread_mutex_unlock(&SchedMutex);

	// If this thread is done executing, send in a signal to the next thread in line.
//	fprintf(stderr, "remainingTime: %d\n", thread->remainingTime);
	if (remainingTime == 0) {
		if (Ready[0] != NULL)
			pthread_cond_signal(&Ready[0]->sleep);
	}
	CurrentTime++;

	fprintf(stderr, "returning %d with current time: %f\n", tid, CurrentTime);

	return CurrentTime;
}

void init_scheduler(int schedType) {
	pthread_mutex_init(&SchedMutex, NULL);
	Scheduler = &fcfs;
}

int scheduleme(float currentTime, int tid, int remainingTime, int tprio) {
	return (*Scheduler)(currentTime, tid, remainingTime, tprio);
}

