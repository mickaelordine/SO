#include "head.h"

int *poolCount, *constants;
transaction *pool;

void readerSigHandler(int signum)
{
    if (signum == SIGUSR1)
    {
        /*sim finita: master -SIGINT-> node -SIGUSR1-> me*/
        if (shmdt(constants) ||
            shmdt(pool) ||
            shmdt(poolCount))
        {
            ERROR("reader detaches");
        }
        exit(0);
    }
}

int main(int argc, char const *argv[])
{
    int poolID, poolCountID, nodeSemID, msgqID, constID;
    int a, i, first, myPID, nodePID;
    message received;
    struct sigaction sa;
    sigset_t mask;

    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigprocmask(SIG_BLOCK, &mask, NULL);
    sa.sa_handler = readerSigHandler;
    sa.sa_mask = mask;
    sa.sa_flags = 0;
    sigaction(SIGUSR1, &sa, NULL);

    nodePID = getppid();
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

        /*get pool*/
        if ((poolID = shmget(nodePID, SO_POOL_SIZE * sizeof(transaction), GETIPC)) == -1)
        {
            ERROR("shmget pool in nr");
        }
        if ((pool = (transaction *)shmat(poolID, NULL, 0)) == (void *)-1)
        {
            ERROR("shmat pool in nr");
        }
        /*get contatore */
        if ((poolCountID = shmget(nodePID * 2, sizeof(int), GETIPC)) == -1)
        {
            ERROR("shmget poolCount in nr");
        }
        if ((poolCount = (int *)shmat(poolCountID, NULL, 0)) == (void *)-1)
        {
            ERROR("shmat poolCount in nr");
        }

        if ((nodeSemID = semget(nodePID, NODESEMSIZE, GETIPC)) == -1)
        {
            ERROR("semget nodeReader")
        }

        /*message queue*/
        if ((msgqID = msgget(nodePID, NEWIPC)) == -1)
        {
            ERROR("msgqget nodeReader");
        }

        /*lo setta a 1 e poi va in wait*/
        if (semctl(nodeSemID, 1, SETVAL, 1) == -1)
        {
            ERROR("semctl nodeSem");
        }

        /*dice al nodo che è pronto*/
        quickTake(nodeSemID, 0);

        /*aspetta che il nodo gli dica di partire*/
        quickWait(nodeSemID, 1);

        /*lo setta a 1, lo usa per accedere in mutex a poolCount*/
        if (semctl(nodeSemID, 1, SETVAL, 1) == -1)
        {
            ERROR("semctl nodeSem");
        }
    }

    /*SIMULAZIONE PARTITA*/
    /*nodeSEM[0]=1
    nodeSEM[0]=1*/

    while (1)
    {
        while (1)
        {
            /*loop per non fermarsi in caso di ricezione di segnale*/
            if (msgrcv(msgqID, &received, sizeof(transaction), nodePID, 0) == -1)
            {
                if (errno != EINTR)
                {
                    /*errore vero e proprio*/
                    ERROR("msgrcv");
                }
            }
            else
            {
                /*abbiamo letto un msg sensato, usciamo dal loop*/
                if (received.type != nodePID)
                {
                    printf("R%d read msg.type = %ld\n", myPID, received.type);
                }
                break;
            }
        }
        a = quickTake(nodeSemID, 1);

        /*prende poolCount*/
        if (*poolCount == SO_POOL_SIZE - 1)
        {
            /*pool piena*/
            received.type = (long) received.transaction.sender;
            /*così l'user sa cosa leggere*/
            if (msgsnd(msgqID, &received, sizeof(transaction), IPC_NOWAIT) == -1)
            {
                ERROR("full pool msgsnd");
            }
            else
            {
                /*
                printf("R%d [N%d]: received %d from U%d, sent him SIGUSR1, and sent msg.type = %ld\n",
                    myPID, nodePID, received.transaction.quantity, received.transaction.sender, received.type);
                */
                kill(received.transaction.sender, SIGUSR1);
                /*dice all'usr pool piena*/
            }

            a = quickTake(nodeSemID, 0);
            /*dice al node pool piena*/

            a = quickFree(nodeSemID, 1);
            /*libera poolCount*/

            a = quickTake(nodeSemID, 0);
            /*pool svuotata, può riprendere*/

            semctl(nodeSemID, 0, SETVAL, 1);
            /*e resetta il sem*/
        }
        else
        {
            pool[(*poolCount)] = received.transaction;
            (*poolCount)++;
            /*la rilascia*/
            a = quickFree(nodeSemID, 1);
        }
    }
}
