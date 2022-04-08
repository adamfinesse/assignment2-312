#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>

#define N 3
// the bound of running readers, any thing beyond this tell the readers to stop using counting semaphore set to N, keep --ing it each time after a wait and if its 0
// make them stuck on wait
/*
This program provides a possible solution for first readers writers problem using mutex and semaphore.
I have used 10 readers and 5 producers to demonstrate the solution. You can always play with these values.
*/

//sem_t wrt;
pthread_mutex_t mutex;
int cnt = 1;
int numreader = 0;
int activereader =0;

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

struct mycountingsem wrt;
//struct mycountingsem rdr;

void *writer(void *wno)
{   
    mycountingsem_wait(&wrt);
    cnt = cnt*2;
    printf("Writer %d modified cnt to %d\n",(*((int *)wno)),cnt);
    mycountingsem_post(&wrt);

}
void *reader(void *rno)
{   
    // Reader acquire the lock before modifying numreader
    pthread_mutex_lock(&mutex);
    numreader++;
    if(numreader == 1) {
        mycountingsem_wait(&wrt); // If this id the first reader, then it will block the writer
    }
    pthread_mutex_unlock(&mutex);

    mycountingsem_wait(&wrt);
    sleep(2);
    //if(mycountingsem_getval(&wrt)==N){
    printf("number of readers active: %d\n",N-mycountingsem_getval(&wrt));
    //}
    printf("Reader %d: read cnt as %d\n",*((int *)rno),cnt);
    mycountingsem_post(&wrt);
    // Reader acquire the lock before modifying numreader
    pthread_mutex_lock(&mutex);
    numreader--;
    if(numreader == 0) {
        mycountingsem_post(&wrt); // If this is the last reader, it will wake up the writer.
    }
    pthread_mutex_unlock(&mutex);
}

int main()
{   

    pthread_t read[10],write[5];
    pthread_mutex_init(&mutex, NULL);
    //sem_init(&wrt,0,1);
    mycountingsem_init(&wrt,N);
    //mycountingsem_init(&rdr,N);

    int a[10] = {1,2,3,4,5,6,7,8,9,10}; //Just used for numbering the producer and consumer

    for(int i = 0; i < 10; i++) {
        pthread_create(&read[i], NULL, (void *)reader, (void *)&a[i]);
    }
    for(int i = 0; i < 5; i++) {
        pthread_create(&write[i], NULL, (void *)writer, (void *)&a[i]);
    }

    for(int i = 0; i < 10; i++) {
        pthread_join(read[i], NULL);
    }
    for(int i = 0; i < 5; i++) {
        pthread_join(write[i], NULL);
    }

    pthread_mutex_destroy(&mutex);
    sem_destroy(&wrt);

    return 0;
    
}
int mycountingsem_getval(struct mycountingsem *p){
    //printf("%d\n",p->val);
    return p->val;
}