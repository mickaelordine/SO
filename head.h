#define _GNU_SOURCE

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <sys/wait.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <signal.h>

/*IPCS*/
#define USERSK 111
#define NODESK 222
#define READERSK 333

#define LEDGERK 770

#define CONSTK 888

#define MASTERSEMK 999
#define MASTERSEMSIZE 2

#define NODESEMSIZE 2

/*UTIL*/
#define REWARDTRANS -1
#define ENDTIME 1
#define ENDSPACE 2
#define ONEALIVE 3
#define MAXSHOW 10
/* ^ sempre pari ^ */
#define NEWIPC (IPC_CREAT | IPC_EXCL | 00666)
#define GETIPC (IPC_CREAT | 00666)

/*COSTANTI*/
#define CONSTSIZE 11

#define SO_REGISTRY_SIZE 101
#define SO_BLOCK_SIZE 10

#define SO_USERS_NUM (constants[0])
#define SO_NODES_NUM (constants[1])
#define SO_BUDGET_INIT (constants[2])
#define SO_REWARD (constants[3])
#define SO_RETRY (constants[4])
#define SO_POOL_SIZE (constants[5])
#define SO_SIM_SEC (constants[6])
#define SO_MIN_TRANS_GEN_NSEC (constants[7])
#define SO_MAX_TRANS_GEN_NSEC (constants[8])
#define SO_MIN_TRANS_PROC_NSEC (constants[9])
#define SO_MAX_TRANS_PROC_NSEC (constants[10])

#define ROUND(x) (int)(x < 0 ? (x - 0.5) : (x + 0.5))

#define ERROR(msg)                                             \
    if (errno)                                                 \
    {                                                          \
        fprintf(stderr,                                        \
                "file:%s; line:%d; pid %ld; errno: %d (%s)\n", \
                __FILE__,                                      \
                __LINE__,                                      \
                (long)getpid(),                                \
                errno,                                         \
                strerror(errno));                              \
        fprintf(stderr, "error in %s\n", msg);                 \
        exit(EXIT_FAILURE);                                    \
    }

typedef struct transaction
{
    struct timespec time;
    pid_t sender;
    pid_t receiver;
    int quantity;
    int reward;
} transaction;

typedef struct libroMastro
{
    transaction libro[SO_REGISTRY_SIZE][SO_BLOCK_SIZE];
    sig_atomic_t blockCounter;
    struct timespec startTime;
    sig_atomic_t deadUsers;
} libroMastro;

typedef struct message
{
    long type;
    struct transaction transaction;
} message;

/*funzioni*/
int quickWait(int semid, int index);
int quickTake(int semid, int index);
int quickFree(int semid, int index);
int randMaxEscl(int max);
int randRangeIncl(int min, int max);
int intContains(int *arr, int size, int val);
int indexOf(int searched, int *arr, int size);
int descending(const void *a, const void *b);
int getReward(transaction t);