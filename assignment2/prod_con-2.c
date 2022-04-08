#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h> 
 
#define SIZE 5
#define NUMB_THREADS 6
#define PRODUCER_LOOPS 2
 
typedef int buffer_t;
buffer_t buffer[SIZE];
int buffer_index;
int fifo_index =-1;
 
pthread_mutex_t buffer_mutex;
/* initially buffer will be empty.  full_sem
   will be initialized to buffer SIZE, which means
   SIZE number of producer threads can write to it.
   And empty_sem will be initialized to 0, so no
   consumer can read from buffer until a producer
   thread posts to empty_sem */
//sem_t full_sem;  /* when 0, buffer is full */
//sem_t empty_sem; /* when 0, buffer is empty. Kind of
           //         like an index for the buffer */
 
 struct mycountingsem{
    int val;
    sem_t gate;
    sem_t mutex;
};
void mycountingsem_init(struct mycountingsem *sem ,int k){
   sem->val = k;
    if(k>=1) sem_init(&sem->gate,0,1);
    else{
        sem_init(&sem->gate,0,0);
    }
    sem_init(&sem->mutex,0,1);
}

void mycountingsem_wait(struct mycountingsem *sem){
    sem_wait(&sem->gate); // do i need &symbol before sem.gate?
    sem_wait(&sem->mutex);
    sem->val--;
    if(sem->val>0){
        sem_post(&sem->gate);
    }
    sem_post(&sem->mutex);
}

void mycountingsem_post(struct mycountingsem *sem){
    sem_wait(&sem->mutex);
    sem->val++;
    if(sem->val==1) sem_post(&sem->gate);
    sem_post(&sem->mutex);
}

struct mycountingsem full_sem0;
struct mycountingsem empty_sem0;

 
void insertbuffer(buffer_t value) {
    if (buffer_index < SIZE) {
        buffer[buffer_index] = value;
        buffer_index = (buffer_index+1)%SIZE;
        //if(fifo_index==0) return;
        //fifo_index--;
    } else {
        printf("Buffer overflow\n");
    }
}
 
buffer_t dequeuebuffer() {
    if (buffer_index >= 0) {
        //buffer_index--;
        //printf("fifo %d\n",fifo_index);
        //printf("buffer %d\n",buffer_index);
        fifo_index = (fifo_index+1)%SIZE;
        return buffer[fifo_index];
        //return buffer[0];
    } else {
        printf("Buffer underflow2\n");
    }
    return 0;
}
 
 
void *producer(void *thread_n) {
    int thread_numb = *(int *)thread_n;
    buffer_t value;
    int i=0;
    while (i++ < PRODUCER_LOOPS) {
        sleep(rand() % 10);
        value = rand() % 100;
        mycountingsem_wait(&full_sem0); // sem=0: wait. sem>0: go and decrement it
        /* possible race condition here. After this thread wakes up,
           another thread could aqcuire mutex before this one, and add to list.
           Then the list would be full again
           and when this thread tried to insert to buffer there would be
           a buffer overflow error */
        pthread_mutex_lock(&buffer_mutex); /* protecting critical section */
        insertbuffer(value);
        pthread_mutex_unlock(&buffer_mutex);
        mycountingsem_post(&empty_sem0); // post (increment) emptybuffer semaphore
        printf("Producer %d added %d to buffer\n", thread_numb, value);
    }
    pthread_exit(0);
}
 
void *consumer(void *thread_n) {
    int thread_numb = *(int *)thread_n;
    buffer_t value;
    int i=0;
    while (i++ < PRODUCER_LOOPS) {
        mycountingsem_wait(&empty_sem0);
        /* there could be race condition here, that could cause
           buffer underflow error */
        pthread_mutex_lock(&buffer_mutex);
        value = dequeuebuffer(value);
        pthread_mutex_unlock(&buffer_mutex);
        mycountingsem_post(&full_sem0); // post (increment) fullbuffer semaphore
        printf("Consumer %d dequeue %d from buffer\n", thread_numb, value);
   }
    pthread_exit(0);
}
 
int main(int argc, int **argv) {
    buffer_index = 0;
 
    pthread_mutex_init(&buffer_mutex, NULL);
    // sem_init(&full_sem, // sem_t *sem
    //          0, // int pshared. 0 = shared between threads of process,  1 = shared between processes
    //          SIZE); // unsigned int value. Initial value
    // sem_init(&empty_sem,
    //          0,
    //          0);
    mycountingsem_init(&full_sem0,SIZE);
    mycountingsem_init(&empty_sem0,0);
    /* full_sem is initialized to buffer size because SIZE number of
       producers can add one element to buffer each. They will wait
       semaphore each time, which will decrement semaphore value.
       empty_sem is initialized to 0, because buffer starts empty and
       consumer cannot take any element from it. They will have to wait
       until producer posts to that semaphore (increments semaphore
       value) */
    pthread_t thread[NUMB_THREADS];
    int thread_numb[NUMB_THREADS];
    int i;
    for (i = 0; i < NUMB_THREADS; ) {
        thread_numb[i] = i;
        pthread_create(thread + i, // pthread_t *t
                       NULL, // const pthread_attr_t *attr
                       producer, // void *(*start_routine) (void *)
                       thread_numb + i);  // void *arg
        i++;
        thread_numb[i] = i;
        // playing a bit with thread and thread_numb pointers...
        pthread_create(&thread[i], // pthread_t *t
                       NULL, // const pthread_attr_t *attr
                       consumer, // void *(*start_routine) (void *)
                       &thread_numb[i]);  // void *arg
        i++;
    }
 
    for (i = 0; i < NUMB_THREADS; i++)
        pthread_join(thread[i], NULL);
 
    pthread_mutex_destroy(&buffer_mutex);
    sem_destroy(&full_sem0);
    sem_destroy(&empty_sem0);
 
    return 0;
}