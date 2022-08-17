#include "head.h"

int myPID, readerPID, poolID, poolCountID, nodeSemID;
int *nodePIDs, *poolCount, *constants, *readerPIDs;
libroMastro *ledger;

transaction rewardTransaction(int rewTot)
{
    transaction t;

    t.quantity = rewTot;
    t.receiver = myPID;
    t.sender = REWARDTRANS;
    t.reward = 0;
    clock_gettime(CLOCK_REALTIME, &t.time);

    return t;
}

struct timespec nodeSleep(int randomSeed)
{
    struct timespec ret;
    srandom(getpid() * randomSeed);
    ret.tv_sec = 0;
    ret.tv_nsec = random() % (SO_MAX_TRANS_PROC_NSEC - SO_MIN_TRANS_PROC_NSEC + 1) + SO_MIN_TRANS_PROC_NSEC;
    return ret;
}

void endNode()
{
    printf("Node %d: pool had %d transactions\n", myPID, *poolCount);

    if (shmctl(poolID, IPC_RMID, NULL) ||
        shmctl(poolCountID, IPC_RMID, NULL) ||
        semctl(nodeSemID, 0, IPC_RMID))
    {
        ERROR("node dealloc")
    };

    if (shmdt(constants) ||
        shmdt(readerPIDs) ||
        shmdt(nodePIDs) ||
        shmdt(ledger))
    {
        ERROR("node detaches");
    }

    kill(readerPID, SIGUSR1);
    waitpid(readerPID, NULL, 0);

    exit(0);
}

void nodeSigHandler(int signum)
{
    if (signum == SIGUSR1)
    {
        /*inviato dal master a fine sim*/
        endNode();
    }
}

int main(int argc, char const *argv[])
{
    int readerPIDsID, masterSemID, nodePIDsID, LMID, constID;
    int i, reward, currentBlock, a;
    
    transaction *pool;
    transaction block[SO_BLOCK_SIZE];

    struct timespec sleepTime;

    struct sigaction sa;
    sigset_t mask;

    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigprocmask(SIG_BLOCK, &mask, NULL);
    sa.sa_mask = mask;
    sa.sa_flags = 0;
    sa.sa_handler = nodeSigHandler;
    sigaction(SIGUSR1, &sa, NULL);

    myPID = getpid();

    /*init*/
    {
        if ((constID = shmget(CONSTK, CONSTSIZE * sizeof(int), GETIPC)) == -1)
        {
            ERROR("const shmget");
        }
        if ((constants = (int *)shmat(constID, NULL, 0)) == (int *)-1)
        {
            ERROR("const shmat");
        }

        if ((masterSemID = semget(MASTERSEMK, MASTERSEMSIZE, GETIPC)) == -1)
        {
            ERROR("semget masterSem");
        }

        quickWait(masterSemID, 0);
        /*è qua quando il master ha fatto nascere tutti U+N*/

        /*array condiviso con i PID dei nodi*/
        if ((nodePIDsID = shmget(NODESK, SO_NODES_NUM * sizeof(int), GETIPC)) == -1)
        {
            ERROR("shmemget nodePIDs");
        }
        if ((nodePIDs = (int *)shmat(nodePIDsID, NULL, 0)) == (void *)-1)
        {
            ERROR("shmat nodePIDs");
        }

        /*array condiviso con i PID dei reader*/
        if ((readerPIDsID = shmget(READERSK, SO_NODES_NUM * sizeof(int), GETIPC)) == -1)
        {
            ERROR("shmemget readerPIDs");
        }
        if ((readerPIDs = (int *)shmat(readerPIDsID, NULL, 0)) == (void *)-1)
        {
            ERROR("shmat readerPIDs");
        }

        /*Libro Mastro*/
        if ((LMID = shmget(LEDGERK, sizeof(libroMastro), GETIPC)) == -1)
        {
            ERROR("shmemget ledger");
        }
        if ((ledger = (libroMastro *)shmat(LMID, NULL, 0)) == (void *)-1)
        {
            ERROR("shmat ledger");
        }

        /*creo la pool*/
        if ((poolID = shmget(myPID, SO_POOL_SIZE * sizeof(transaction), NEWIPC)) == -1)
        {
            ERROR("shmget pool");
        }

        if ((pool = (transaction *)shmat(poolID, NULL, 0)) == (void *)-1)
        {
            ERROR("shmat pool");
        }
        bzero(pool, sizeof(transaction) * SO_POOL_SIZE);

        /*contatore di quante trans nella pool*/
        if ((poolCountID = shmget(myPID * 2, sizeof(int), NEWIPC)) == -1)
        {
            ERROR("shmget poolCount");
        }
        if ((poolCount = (int *)shmat(poolCountID, NULL, 0)) == (void *)-1)
        {
            ERROR("shmat poolCount");
        }
        *poolCount = 0;

        /*semaforo di sync tra node e reader*/
        if ((nodeSemID = semget(myPID, NODESEMSIZE, NEWIPC)) == -1)
        {
            ERROR("semget node")
        }

        /*lo setta a 1 e poi va in wait, quando il reader ha finito lo mette a 0*/
        if (semctl(nodeSemID, 0, SETVAL, 1) == -1)
        {
            ERROR("semctl nodeSem");
        }


    /*generazione reader*/
    switch (readerPID = fork())
    {
    case -1:
        ERROR("node/reader fork");

    case 0:
        execl("nodeReader", "newnodereader.c", NULL);
        break;

    default:
        readerPIDs[indexOf(myPID, nodePIDs, SO_NODES_NUM)] = readerPID;
        break;
    }

    quickWait(nodeSemID, 0);
    /*il reader è pronto

    lo dice al master*/
    quickTake(masterSemID, 1);
    /* ... e aspetta che tutti U+N+NR siano pronti*/
    quickWait(masterSemID, 1);
}
    /*SIMULAZIONE PARTITA*/

    /*lo setta a 1, il reader lo prende quando pool piena*/
    if (semctl(nodeSemID, 0, SETVAL, 1) == -1)
    {
        ERROR("semctl nodeSem");
    }
    /*=> da il via al nodeReader*/
    quickTake(nodeSemID, 1);

    while (1)
    {
        if (!quickTake(nodeSemID, 1))
        {
            /*prendiamo pC*/
            if (*poolCount >= SO_BLOCK_SIZE)
            {
                /*possiamo creare un blocco*/
                /*      POOL        */
                reward = 0;
                for (i = 0; i < SO_BLOCK_SIZE - 1; i++)
                {
                    block[i] = pool[i];
                    reward += getReward(pool[i]);
                }
                block[i] = rewardTransaction(reward);

                /*svuota la pool*/
                for (i = 0; i < (*poolCount - (SO_BLOCK_SIZE - 1)); i++)
                {
                    pool[i] = pool[i + (SO_BLOCK_SIZE - 1)];
                }
                *poolCount -= (SO_BLOCK_SIZE - 1);

                /*rilascia pC*/
                a = quickFree(nodeSemID, 1);
                if (semctl(nodeSemID, 0, GETVAL) == 0)
                {
                    /*se il reader sta aspettando che il node svuoti la pool piena*/
                    quickFree(nodeSemID, 0);
                    /*gli dice che l'ha fatto*/
                }

                /*      LEDGER INSERT      */
                a = quickTake(masterSemID, 0);
                if (ledger->blockCounter == SO_REGISTRY_SIZE)
                {
                    /*il libro è pieno, non deve più fare nulla
                    non faccio Free(nodeSem, 1) perchè così sia
                    io che il reader non famo più nulla*/

                    /*segno che la pool non è stata svuotata*/
                    *poolCount += (SO_BLOCK_SIZE - 1);

                    /*dice al master LM pieno*/
                    kill(getppid(), SIGUSR1);
                    a = quickFree(masterSemID, 0);
                    /*aspetto il segnale del master*/
                    pause();
                }
                else
                {
                    /*posso inserire il blocco*/
                    sleepTime = nodeSleep(*poolCount);
                    nanosleep(&sleepTime, NULL);

                    currentBlock = ledger->blockCounter;
                    for (i = 0; i < SO_BLOCK_SIZE; i++)
                    {
                        ledger->libro[currentBlock][i] = block[i];
                    }
                    ledger->blockCounter++;
                    /*printf("N: block inserted: BC = %d\n", ledger->blockCounter);*/


                    if (ledger->blockCounter == SO_REGISTRY_SIZE)
                    {
                        /*come sopra*/
                        *poolCount += (SO_BLOCK_SIZE - 1);
                        kill(getppid(), SIGUSR1);
                        a = quickFree(masterSemID, 0);
                        pause();
                    }
                    else
                    {
                        /*mi stacco dal ledger*/
                        a = quickFree(masterSemID, 0);
                    }
                }
            }
            else
            {
                /*non ho abbastanza transazioni*/
                a = quickFree(nodeSemID, 1);
            }
        }
    }
}