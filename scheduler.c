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

static void sortQueueRemainingTimeIncreasing ()
{
	int i;
	int flag = 0;
	for(i = 0; i < 5; i++)
	{
		while(!flag)
		{
			struct node *head = Ready[i];
			struct node *prev = head;
			flag = 1;
			while(head != NULL && head->next != NULL)
			{
				if(head->remainingTime > head->next->remainingTime)
				{
					if(prev == head)
					{
						struct node *tmp = head->next;
						head->next = tmp->next;
						tmp->next = head;
						Ready[i] = tmp;			
					}
					else {
						struct node *tmp = head->next;
						head->next = head->next->next;
						tmp->next = head;
						head = tmp;
						prev->next = head;
					}
					flag = 0;
				}
				prev = head;
				head = head->next;
			}
		}
		while(!flag)
		{
			struct node *head = Ready[i];
			struct node *prev = head;
			flag = 1;
			while(head != NULL && head->next != NULL)
			{
				if(head->remainingTime == head->next->remainingTime && head->currentTime > head->next->currentTime)
				{
					if(prev == head)
					{
						struct node *tmp = head->next;
						head->next = tmp->next;
						tmp->next = head;
						Ready[i] = tmp;			
					}
					else {
						struct node *tmp = head->next;
						head->next = head->next->next;
						tmp->next = head;
						head = tmp;
						prev->next = head;
					}
					flag = 0;
				}
				prev = head;
				head = head->next;
			}
		}
	}
	
}

static void addQueueRemainingTimeIncreasing (struct node* toAdd) {
	struct node* head = Ready[toAdd->tprio];
	struct node* prev = head;
	toAdd->next = NULL;

	if (head == NULL) {
		Ready[toAdd->tprio] = toAdd;
		return;
	}
	while (head != NULL && head->remainingTime < toAdd->remainingTime) {
		prev = head;
		head = head->next;
	}

	while (head != NULL && head->remainingTime == toAdd->remainingTime &&  head->currentTime < toAdd->currentTime) 
	{
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
		if(Executing == NULL) Executing = Ready[0];
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
		Executing = NULL;
	} else {
		CurrentTime++;
		// If the thread is not ready to run, 'yield' for some amount of time.
		// For instance, the CurrentTime could be 12, yet the arrival time is
		// 12.1. As we're assuming non-divisible time slots, we should round up
		// to 13.
		if (CurrentTime < thread->currentTime) {
			CurrentTime += ceil(-CurrentTime + thread->currentTime);
		}
		Executing = thread;
	}
	pthread_mutex_unlock(&SchedMutex);

	// If this thread is done executing, send in a signal to the next thread in line.
	if (remainingTime == 0) {
		if (Ready[0] != NULL)
			pthread_cond_signal(&Ready[0]->sleep);
	}

	return CurrentTime;
}

int inQueue (int tid)
{
	int i;
	for(i = 0; i < 5; i++)
	{
		struct node *head = Ready[i];

		while(head != NULL)
		{
			if(head->tid == tid)
				return 1;
			head = head->next;
		}
	}
	return 0;
}

int srtf(float currentTime, int tid, int remainingTime, int tprio) {
	// Acquire mutex.
	pthread_mutex_lock(&SchedMutex);

	//printf("Entered srtf [%d]\n", tid);
	// Identify if this is the first time the thread is requesting to be scheduled.
	// If it does not match with the thread "Executing", then it is.
	struct node* thread = NULL;
	if (!inQueue(tid)) {
		//fprintf(stderr, " (first arrival)\n");
		// Create a 'struct node' value to represent this thread.
		thread = malloc(sizeof(struct node));
		thread->remainingTime = remainingTime;
		thread->tid = tid;
		thread->currentTime = currentTime;
		thread->tprio = 0;
		pthread_cond_init(&thread->sleep, NULL);
		addQueueRemainingTimeIncreasing(thread);
	} else {
		if(Executing == NULL) Executing = Ready[0];
		thread = Executing;
		thread->remainingTime = remainingTime;
		thread->currentTime = CurrentTime + 1;
		sortQueueRemainingTimeIncreasing();
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
		Executing = NULL;
	} else {
		CurrentTime++;
		// If the thread is not ready to run, 'yield' for some amount of time.
		// For instance, the CurrentTime could be 12, yet the arrival time is
		// 12.1. As we're assuming non-divisible time slots, we should round up
		// to 13.
		if (CurrentTime < thread->currentTime) {
			CurrentTime += ceil(-CurrentTime + thread->currentTime);
		}
		Executing = thread;
	}
	pthread_mutex_unlock(&SchedMutex);

	// If this thread is done executing, send in a signal to the next thread in line.
	//printf("remainingTime [%d] = %d\n", tid, remainingTime);
	if (remainingTime == 0) {
		//Executing = NULL;
		if (Ready[0] != NULL )
			pthread_cond_signal(&Ready[0]->sleep);
	}

	return CurrentTime;
}

int pbs(float currentTime, int tid, int remainingTime, int tprio) {

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

//int main ()
//{
//  struct node* t1 = malloc(sizeof(struct node));
//  struct node* t2 = malloc(sizeof(struct node));
//  struct node* t3 = malloc(sizeof(struct node));
//  struct node* t4 = malloc(sizeof(struct node));
//  struct node* t5 = malloc(sizeof(struct node));
//
//  t1->tid = 1;
//  t1->tprio = 0;
//  t1->remainingTime = 10.0f;
//  t1->currentTime = 10.0f;
//  
//  t2->tid = 2;
//  t2->tprio = 0;
//  t2->remainingTime = 20.0f;
//  t2->currentTime = 20.0f;
//  
//  t3->tid = 3;
//  t3->tprio = 0;
//  t3->remainingTime = 20.0f;
//  t3->currentTime = 10.0f;
//  
//  t4->tid = 4;
//  t4->tprio = 0;
//  t4->remainingTime = 30.0f;
//  t4->currentTime = 00.0f;
//  
//  t5->tid = 5;
//  t5->tprio = 0;
//  t5->remainingTime = 10.0f;
//  t5->currentTime = 30.0f;
//
//  addQueueCurrentTimeIncreasing(t1);
//  struct node* head = Ready[0];
//  while(head != NULL)
//  {
//      printf("TID %d Reamaining %d Current %g\n", head->tid, head->remainingTime, head->currentTime);
//      head = head->next;
//  }
//  printf("\n");
//  addQueueCurrentTimeIncreasing(t2);
//  head = Ready[0];
//  while(head != NULL)
//  {
//      printf("TID %d Reamaining %d Current %g\n", head->tid, head->remainingTime, head->currentTime);
//      head = head->next;
//  }
//  printf("\n");
//  addQueueCurrentTimeIncreasing(t3);
//  head = Ready[0];
//  while(head != NULL)
//  {
//      printf("TID %d Reamaining %d Current %g\n", head->tid, head->remainingTime, head->currentTime);
//      head = head->next;
//  }
//  printf("\n");
//  addQueueCurrentTimeIncreasing(t4);
//  head = Ready[0];
//  while(head != NULL)
//  {
//      printf("TID %d Reamaining %d Current %g\n", head->tid, head->remainingTime, head->currentTime);
//      head = head->next;
//  }
//  printf("\n");
//  addQueueCurrentTimeIncreasing(t5);
//  head = Ready[0];
//  while(head != NULL)
//  {
//      printf("TID %d Reamaining %d Current %g\n", head->tid, head->remainingTime, head->currentTime);
//      head = head->next;
//  }
//  printf("\n");
//
//  sortQueueRemainingTimeIncreasing();
//  head = Ready[0];
//  while(head != NULL)
//  {
//      printf("TID %d Reamaining %d Current %g\n", head->tid, head->remainingTime, head->currentTime);
//      head = head->next;
//  }
//
//  return 0;
//}
