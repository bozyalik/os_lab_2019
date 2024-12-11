#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;

void* thread1_function(void* arg) {
	printf("Thread 1: Trying to lock mutex1...\n");
	pthread_mutex_lock(&mutex1);
	printf("Thread 1: mutex1 locked.\n");
	
	// do something
	sleep(1);
	
	printf("Thread 1: Trying to lock mutex2...\n");
	pthread_mutex_lock(&mutex2);
	printf("Thread 1: mutex2 locked.\n");
	
	printf("Thread 1: In critical section.\n");
	
	pthread_mutex_unlock(&mutex2);
	pthread_mutex_unlock(&mutex1);
	return NULL;
}

void* thread2_function(void* arg) {
	printf("Thread 2: Trying to lock mutex2...\n");
	pthread_mutex_lock(&mutex2);
	printf("Thread 2: mutex2 locked.\n");
	
	// do anything else
	sleep(1);
	
	printf("Thread 2: Trying to lock mutex1...\n");
	pthread_mutex_lock(&mutex1);
	printf("Thread 2: mutex1 locked.\n");
	
	printf("Thread 2: In critical section.\n");
	
	pthread_mutex_unlock(&mutex2);
	pthread_mutex_unlock(&mutex1);
	return NULL;
}

int main() {
	pthread_t thread1, thread2;
	
	if (pthread_create(&thread1, NULL, thread1_function, NULL)) {
		perror("pthread_create");
		return 1;
	}
	
	if (pthread_create(&thread2, NULL, thread2_function, NULL)) {
		perror("pthread_create");
		return 1;
	}
	
	pthread_join(thread1, NULL);
	pthread_join(thread2, NULL);
	
	printf("Main: Both threads finished.\n");
	
	pthread_mutex_destroy(&mutex1);
	pthread_mutex_destroy(&mutex2);
	
	return 0;
}