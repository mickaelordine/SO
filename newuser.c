#include "head.h"

int myPID, transSent, totSent;
int *userPIDs, *nodePIDs, *constants, *readerPIDs;
libroMastro *ledger;

struct timespec userSleep(int randomSeed)
{
    struct timespec ret;
    srandom(getpid() * randomSeed);
    ret.tv_sec = 0;
    ret.tv_nsec = random() % (SO_MAX_TRANS_GEN_NSEC - SO_MIN_TRANS_GEN_NSEC + 1) + SO_MIN_TRANS_GEN_NSEC;
    return ret;
}

int calculateReceived()
{
    int i, j, received;
    double toRound;
    transaction current;
    received = 0;
    for (i = 0; i < ledger->blockCounter; i++)
    {
        for (j = 0; j < SO_BLOCK_SIZE - 1; j++)
        {
            current = ledger->libro[i][j];
            if (current.receiver == myPID)
            {
                received += (current.quantity - getReward(current));
            }
        }
    }
    return received;
}

pid_t getDest()
{
    int found, destPID;
    found = 0;
    while (!found)
    {
        if (ledger->deadUsers == SO_USERS_NUM - 1)
        {
            return -1;
        }
        destPID = userPIDs[randMaxEscl(SO_USERS_NUM)];
        if (destPID != myPID && destPID > 0)
        {
            /*user vivo e != me*/
            found = 1;
        }
    }
    return destPID;
}

pid_t getNode()
{
    return nodePIDs[randMaxEscl(SO_NODES_NUM)];
}

void endUser(int how)
{
    int myIndex;
    myIndex = indexOf(myPID, userPIDs, SO_USERS_NUM);

    if (userPIDs[myIndex] > 0)
    {
        userPIDs[myIndex] = -myPID;
        /*segna che è morto*/
    }

    /*
    how == -1 => mai partito
    how != -1 => morte naturale
    */

    if (shmdt(constants) ||
        shmdt(userPIDs) ||
        shmdt(readerPIDs) ||
        shmdt(nodePIDs) ||
        shmdt(ledger))
    {
        ERROR("user detaches");
    }

    if (how == -1)
    {
        printf("U#%d ABORTED", myPID);
    }
    exit(0);
}

void sigUSR1Handler(int signum, siginfo_t *info, void *vp)
{
    int sender, node, qID;
    message received;
    if (signum == SIGUSR1)
    {
        sender = info->si_pid; /* Sending process ID from the info of the signal recived*/
        if (sender == getppid())
        {
            /*master dice fine sim*/
            endUser(transSent);
        }
        else
        {
            /*arriva dal reader per dire pool full*/
            node = nodePIDs[indexOf(sender, readerPIDs, SO_NODES_NUM)];
            qID = msgget(node, GETIPC);
            if (msgrcv(qID, &received, sizeof(transaction), myPID, IPC_NOWAIT) == -1) //con IPC_NOWAIT permettiamo una non blockingsend
                                                                                      //che genera EAGAIN in caso di errore
            {
                    ERROR("user msgrcv");

            }
            else
            {
                if (received.transaction.sender == myPID)
                {
                    /*ulteriore check di sicurezza*/
                    totSent -= received.transaction.quantity;
                    /*
                    printf("U%d : sent %d to R%d [N%d] 's full pool\n",
                           myPID, received.transaction.quantity, reader, node);
                    */
                }
                else
                {
                    /*in teoria non dovremmo mai essere qua*/
                    printf(" something wrong ");
                }
            }
        }
    }
}

void userSigHandler(int signum)
{
    message toSend;
    int balance, qID;
    struct timespec sleepTime;
    switch (signum)
    {
    case SIGALRM:
        /*l'user non ha mai iniziato*/
        endUser(-1);
        break;

    case SIGUSR2:
    /*send random trans*/
        balance = SO_BUDGET_INIT + calculateReceived() - totSent;
        if (balance >= 2)
        {
            toSend.transaction.receiver = getDest();
            if (toSend.transaction.receiver == -1)
            {
                printf("U#%d is last one alive\n", myPID);
                endUser(transSent);
            }
            toSend.transaction.quantity = randRangeIncl(2, balance);
            clock_gettime(CLOCK_REALTIME, &toSend.transaction.time);
            toSend.transaction.sender = myPID;
            toSend.transaction.reward = SO_REWARD;
            toSend.type = getNode();

            if ((qID = msgget(toSend.type, GETIPC)) == -1)
            {
                ERROR("user msgqget");
            }
            else if (msgsnd(qID, &toSend, sizeof(transaction), IPC_NOWAIT) == -1)
            {
                ERROR("user msgsend");
            }
            else
            {
                printf("\tU %d sent transaction to U %d \n", myPID, toSend.transaction.receiver);
                totSent += toSend.transaction.quantity;
                sleepTime = userSleep(ledger->blockCounter);
                nanosleep(&sleepTime, NULL);
                transSent++;
            }
        }
        else
        {
            printf("\tU %d has not enough money to send a transaction\n", myPID);
        }
        printf(" +  +  +  +  +  +  +  +  +  +  +  +  +  +  + \n");
        break;

    default:
        ERROR("user sigHandler");
        break;
    }
}

void newAlarmHandler(int signum)
{}

int main(int argc, char const *argv[])
{
    int masterSemID, userPIDsID, nodePIDsID, LMID, constID, readerPIDsID, targetQID;
    int targetNode, balance, lives, qty;
    message toSend;
    struct timespec sleepTime;
    struct sigaction sa;
    sigset_t mask;

    myPID = getpid();
    srand(myPID);

    sigemptyset(&mask);
    sigaddset(&mask, SIGINT); // blocchiamo la ricezione del CTRL+C
    sigprocmask(SIG_BLOCK, &mask, NULL); //SIG_BLOCK -> la nuova signal mask diventa l'OR binario di 
                                         //quella corrente con quella specificata da &mask, si poteva usare SIG_SETMASK
    sa.sa_mask = mask;
    sa.sa_flags = 0; // No special behaviour
    sa.sa_handler = userSigHandler; //sa.handler contains only one parameter
    sigaction(SIGALRM, &sa, NULL); //per time expired
    sigaction(SIGUSR2, &sa, NULL); //personalizzato, usato per mandare una transazione casuale
    sa.sa_flags = SA_SIGINFO; //from keyboard
    sa.sa_sigaction = sigUSR1Handler; //sa.sigaction contains 3 params (int, siginfo_t *, void *)
    sigaction(SIGUSR1, &sa, NULL); //personalizzato

    /*inizializzazione*/
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
        /*aspettano che il master figli tutti U+N*/
        quickWait(masterSemID, 0);

        /*array condiviso con i PID degli user*/
        if ((userPIDsID = shmget(USERSK, SO_USERS_NUM * sizeof(int), GETIPC)) == -1)
        {
            ERROR("shmemget userPIDs");
        }
        if ((userPIDs = (int *)shmat(userPIDsID, NULL, 0)) == (void *)-1)
        {
            ERROR("shmat userPIDs");
        }

        /*array condiviso con i PID dei nodi*/
        if ((nodePIDsID = shmget(NODESK, SO_NODES_NUM * (sizeof(int)), GETIPC)) == -1)
        {
            ERROR("shmemget nodePIDs")
        };
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
        alarm(1);
        /*
        dice al master che ha finito*/
        quickTake(masterSemID, 1);

        /* ... e aspetta che tutti U+N+NR siano pronti*/
        quickWait(masterSemID, 1);
        /*SIMULAZIONE PARTITA*/
    }

    transSent = 0;
    totSent = 0;
    lives = SO_RETRY;

    while (1)
    {
        balance = SO_BUDGET_INIT + calculateReceived() - totSent;
        if (balance >= 2)
        {
            toSend.transaction.receiver = getDest();
            if (toSend.transaction.receiver == -1)
            {
                printf("U#%d is last one alive\n", myPID);
                endUser(transSent);
            }
            qty = randRangeIncl(2, balance);

            toSend.transaction.quantity = qty;
            clock_gettime(CLOCK_REALTIME, &toSend.transaction.time);            
            toSend.transaction.sender = myPID;
            toSend.transaction.reward = SO_REWARD;
            targetNode = getNode();
            toSend.type = targetNode;

            if ((targetQID = msgget(targetNode, GETIPC)) == -1)
            {
                ERROR("user msgqget");
            }
            else
            {
                if (msgsnd(targetQID, &toSend, sizeof(transaction), IPC_NOWAIT) == -1)
                {
                    if(errno == EAGAIN){
                        /*printf("msgq is full, waiting\n");*/
                        sleepTime = userSleep(SO_USERS_NUM);
                        nanosleep(&sleepTime, NULL);
                    } else ERROR("user msgsend");
                }
                else
                {
                    /*messaggio inviato con successo*/
                    if (transSent == 0)
                    {
                        sa.sa_handler = newAlarmHandler;
                        sigaction(SIGALRM, &sa, NULL);
                    }
                    /*printf("/ U%d: [%d/%d]-> U%d /", myPID, qty, balance, toSend.transaction.receiver);*/
                    dprintf(3, " - send - ");
                    totSent += qty;
                    lives = SO_RETRY;
                    sleepTime = userSleep(qty);
                    nanosleep(&sleepTime, NULL);
                    transSent++;
                }
            }
        }
        else
        {
            lives--;
            if (!lives)
            {
                /* printf("U#%d: dead after %d trans, balance = %d\n",
                    myPID, transSent, balance); */
                endUser(transSent);
            }
        }
    }
}
