#ifndef INCLUDESH
#define INCLUDESH

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <errno.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>



#define EXIT_ON_ERROR if (errno) {fprintf(stderr,             \
              "file:%s; line:%d; pid %ld; errno: %d (%s)\n",  \
              __FILE__,                                       \
              __LINE__,                                       \
              (long) getpid(),                                \
              errno,                                          \
              strerror(errno)); exit(EXIT_FAILURE);}


#define SO_REGISTRY_SIZE 1000
#define SO_BLOCK_SIZE 10
#define IPC_NUM 6


union semun {
  int val;    /* Value for SETVAL */
  struct semid_ds *buf;    /* Buffer for IPC_STAT, IPC_SET */
  unsigned short  *array;  /* Array for GETALL, SETALL */
  struct seminfo  *__buf;  /* Buffer for IPC_INFO */
};

/*struttura che definisce la transazione*/
typedef struct trans{
    time_t *time;
    int sender;
    int receiver;
    double quantita;
    double reward;
}trans;

/*struttura blocco che genera sostanzialmente una matrice*/
struct block{
    trans transBlock[SO_BLOCK_SIZE];
};

/*struttura per passare messaggi tra nodo e utente*/
typedef struct my_msg{
    long type;
    struct trans transition;
}my_msg;

/*libro mastro*/
struct libro{
    struct block registro[SO_REGISTRY_SIZE];
    int blockCounter;
};



int get_const(char * c); 

/*funzioni semafori*/
void printIntArray(int * arr, int size);
struct sembuf newSemOp(int index, int op, int flags);
struct sembuf quickOp(int index, const char * shortcut);
int quickFree(int id, int index);
int quickTake(int id, int index);
int quickWait(int id, int index);
int quickTakeN(int id, int index, int count);
/*funzioni semafori*/

/*funzioni utenti*/
int checkIsNotMe(int* arrayWithUserId);
int getDest(int * dests);
int utente();

/*funzioni utenti*/



/*
    KEYS:
    [0]->array con pid degli utenti
    [1]->array con pid dei nodi
    [2]->libro mastro
    [3]->message queue
    [4]->semaforo sblocco utenti
    [5]->semaforo blocco utenti
    [6]->semaforo accesso libro mastro
    */

#endif