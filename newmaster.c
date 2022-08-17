#include "head.h"

int *userPIDs, *nodePIDs, *readerPIDs;
int readerPIDsID, masterSemID, userPIDsID, nodePIDsID, LMID, constID;
int *constants, finished;
libroMastro *ledger;

void readConstants()
{
    int num, i;
    FILE *f;
    char *nome;

    if ((constID = shmget(CONSTK, CONSTSIZE * sizeof(int), NEWIPC)) == -1)
    {
        ERROR("const shmget");
    }
    if ((constants = (int *)shmat(constID, NULL, 0)) == (int *)-1)
    {
        ERROR("const shmat");
    }

    f = fopen("const.txt", "r");

    nome = malloc(30 * sizeof(char));
    for (i = 0; i < CONSTSIZE; i++)
    {
        fscanf(f, "%s %i", nome, &num);
        constants[i] = num;
    }
    free(nome);
}

void calculateBalances()
{
    /*int topUsers[MAXSHOW][2], topNodes[MAXSHOW][2];*/
    int *userBalances, *nodeBalances, *userSorted, *nodeSorted;
    int i, j, sndIndex, rcvIndex, nodeIndex, bal, ind, max;
    transaction current;
    double received;
    int minCount, maxCount, maxPID, minPID;

    /*  inizializzazione array balance  */
    userBalances = malloc(SO_USERS_NUM * sizeof(int));
    nodeBalances = malloc(SO_NODES_NUM * sizeof(int));

    for (i = 0; i < SO_USERS_NUM; i++)
    {
        userBalances[i] = SO_BUDGET_INIT;
    }
    bzero(nodeBalances, SO_NODES_NUM * sizeof(int));

    /*calcolo balances*/
    for (i = 0; i < ledger->blockCounter; i++)
    {
        for (j = 0; j < SO_BLOCK_SIZE - 1; j++)
        {
            /* BLKSZ-1 perchè l'ultima è quella di reward del nodo */
            current = ledger->libro[i][j];
            sndIndex = indexOf(current.sender, userPIDs, SO_USERS_NUM);
            rcvIndex = indexOf(current.receiver, userPIDs, SO_USERS_NUM);
            if (sndIndex != -1 && rcvIndex != -1)
            {
                userBalances[sndIndex] -= current.quantity;
                userBalances[rcvIndex] += (current.quantity - getReward(current));
            }
        }
        current = ledger->libro[i][SO_BLOCK_SIZE - 1];
        nodeIndex = indexOf(current.receiver, nodePIDs, SO_NODES_NUM);
        if (nodeIndex != -1)
        {
            nodeBalances[nodeIndex] += current.quantity;
        }
    }

    /*sorting*/
    {
        userSorted = malloc(SO_USERS_NUM * sizeof(int));
        nodeSorted = malloc(SO_NODES_NUM * sizeof(int));
        for (i = 0; i < SO_USERS_NUM; i++)
        {
            userSorted[i] = userBalances[i];
        }
        qsort(userSorted, SO_USERS_NUM, sizeof(int), descending);
        for (i = 0; i < SO_NODES_NUM; i++)
        {
            nodeSorted[i] = nodeBalances[i];
        }
        qsort(nodeSorted, SO_NODES_NUM, sizeof(int), descending);
    }

    /*print USER*/
    printf("\tUSER BALANCES: \n");
    if (SO_USERS_NUM > MAXSHOW)
    {
        minCount = 0;
        maxCount = 0;

        i = 0;
        while (userSorted[i] == userSorted[0])
        {
            maxCount++;
            i++;
        }
        maxPID = indexOf(userSorted[0], userBalances, SO_USERS_NUM);
        maxPID = userPIDs[maxPID];

        i = SO_USERS_NUM - 1;
        while (userSorted[i] == userSorted[SO_USERS_NUM - 1])
        {
            minCount++;
            i--;
        }
        minPID = indexOf(userSorted[SO_USERS_NUM - 1], userBalances, SO_USERS_NUM);
        minPID = userPIDs[minPID];

        printf("RICHEST USER: %d, balance = %d (%d users have this balance)\n", abs(maxPID), userSorted[0], maxCount);
        printf("POOREST USER: %d, balance = %d (%d users have this balance)\n\n", abs(minPID), userSorted[SO_USERS_NUM - 1], minCount);
    }
    else
    {
        max = ((int)ceil((double)SO_USERS_NUM / 2));
        for (i = 0; i < max; i++)
        {
            if (i + max >= SO_USERS_NUM)
            {
                printf("U#%d -> %d\n", userPIDs[i], userBalances[i]);
            }
            else
            {
                printf("U#%d -> %d\t\tU#%d -> %d\n",
                       userPIDs[i], userBalances[i],
                       userPIDs[i + max], userBalances[i + max]);
            }
        }
    }

    /*print NODE*/
    printf("\tNODE BALANCES:\n");
    if (SO_NODES_NUM > MAXSHOW)
    {
        minCount = 0;
        maxCount = 0;
        i = 0;
        while (nodeSorted[i] == nodeSorted[0])
        {
            maxCount++;
            i++;
        }
        maxPID = indexOf(nodeSorted[0], nodeBalances, SO_NODES_NUM);
        maxPID = nodePIDs[maxPID];

        i = SO_NODES_NUM - 1;
        while (nodeSorted[i] == nodeSorted[SO_NODES_NUM - 1])
        {
            minCount++;
            i--;
        }
        minPID = indexOf(nodeSorted[SO_NODES_NUM - 1], nodeBalances, SO_NODES_NUM);
        minPID = nodePIDs[minPID];

        printf("RICHEST NODE: %d, balance = %d (%d nodes have this balance)\n", maxPID, nodeSorted[0], maxCount);
        printf("POOREST NODE: %d, balance = %d (%d nodes have this balance)\n\n", minPID, nodeSorted[SO_NODES_NUM - 1], minCount);
    }
    else
    {
        max = ((int)ceil((double)SO_NODES_NUM / 2));
        for (i = 0; i < max; i++)
        {
            if (i + max >= SO_NODES_NUM)
            {
                printf("N#%d -> %d\n", nodePIDs[i], nodeBalances[i]);
            }
            else
            {
                printf("N#%d -> %d\t\tN#%d -> %d\n",
                       nodePIDs[i], nodeBalances[i],
                       nodePIDs[i + max], nodeBalances[i + max]);
            }
        }
    }
    free(userBalances);
    free(nodeBalances);
}

void endMaster(int how)
{
    int i;
    sigset_t mask;

    sigemptyset(&mask);
    sigaddset(&mask, SIGALRM);
    sigprocmask(SIG_BLOCK, &mask, NULL);

    if (!finished)
    {
        finished = 1;
        switch (how)
        {
        case ENDTIME:
            printf("MASTER: end of sim because TIME EXPIRED\n");
            break;
        case ENDSPACE:
            printf("MASTER: end of sim because FULL LEDGER\n");
            break;
        case ONEALIVE:
            printf("MASTER: end of sim because DEAD USERS\n");
            break;
        }
        if (shmctl(userPIDsID, IPC_RMID, NULL) ||
            shmctl(nodePIDsID, IPC_RMID, NULL) ||
            semctl(masterSemID, 0, IPC_RMID) ||
            shmctl(LMID, IPC_RMID, NULL) ||
            shmctl(constID, IPC_RMID, NULL) ||
            shmctl(readerPIDsID, IPC_RMID, NULL))
        {
            ERROR("master dealloc");
        }

        printf("BlockCounter = %d/%d - DeadUsers = %d/%d\n\n",
               ledger->blockCounter, SO_REGISTRY_SIZE,
               ledger->deadUsers, SO_USERS_NUM);

        for (i = 0; i < SO_USERS_NUM; i++)
        {
            kill(userPIDs[i], SIGUSR1);
            waitpid(userPIDs[i], NULL, 0);
        }
        printf("\tMASTER: all users dead\n\n");

        for (i = 0; i < SO_NODES_NUM; i++)
        {
            kill(nodePIDs[i], SIGUSR1);
            waitpid(nodePIDs[i], NULL, 0);
            if (msgctl(msgget(nodePIDs[i], GETIPC), IPC_RMID, NULL))
            {
                ERROR("master q dealloc");
            }
        }
        printf("\tMASTER: all nodes dead\n\n");
        calculateBalances();
        printf("\tMASTER: bye bye\n");
        exit(0);
    }
}

void sigChldHandler(int signum, siginfo_t *info, void *vp)
{
    int sender;
    if (signum == SIGCHLD)
    {
        sender = info->si_pid;
        if (waitpid(sender, NULL, WNOHANG))
        {
            if (intContains(userPIDs, SO_USERS_NUM, sender))
            {
                /*sigchld da user morto*/
                ledger->deadUsers++;
                if (ledger->deadUsers >= SO_USERS_NUM - 1)
                {
                    endMaster(ONEALIVE);
                }
            }
        }
    }
}

void masterSigHandler(int signum)
{
    struct timespec sleepTime;
    int ind, dest;
    ind = -1;
    dest = -1;
    if (signum == SIGINT)
    {
        
        printf(" +  +  +  +  +  +  +  +  +  +  +  +  +  +  + \n");
        printf("\tSelect the index of the user you want to send \n\tthe transaction (must be between 0 and %d)\n\t -1 to resume the simulation: ", SO_USERS_NUM - 1);
        while (1)
        {
            scanf("%d", &ind);
            if (ind == -1)
            {
                printf("resuming ...\n");
                break;
            }
            if (ind < 0 || ind >= SO_USERS_NUM - 1)
            {
                printf("\t%d is not a valid index, try another value\n", ind);
            }
            else
            {
                dest = userPIDs[ind];
                if (dest < 0)
                {
                    printf("\tthe requested user is already dead, try another value\n");
                    ind = -1;
                }
                else
                {
                    printf("\tRequesting a transaction send to User %d ...", dest);
                    kill(userPIDs[ind], SIGUSR2);
                    sleepTime.tv_sec = 0;
                    sleepTime.tv_nsec = 10000;
                    /*lasciamo il tempo all'user di elaborare*/
                    break;
                }
            }
        }
    }
    if (signum == SIGUSR1)
    {
        quickTake(masterSemID, 0);
        endMaster(ENDSPACE);
    }
    if (signum == SIGALRM)
    {
        endMaster(ENDTIME);
    }
}

int main(int argc, char const *argv[])
{
    int i, childPID, a;
    struct timespec sleepTime;
    struct sigaction sa;
    sigset_t mask;
    siginfo_t *info;

    readConstants();
    {
        /*
        2 celle:
        la prima fa partire gli user e i nodi
        la seconda gli user quando è tutto pronto
        */
        if ((masterSemID = semget(MASTERSEMK, MASTERSEMSIZE, NEWIPC)) == -1)
        {
            ERROR("semget masterSem");
        }
        /*setto la prima a 1,
        la libererò dal master quando tutti U+N (che sono in wait) sono nati, così partono*/
        if ((semctl(masterSemID, 0, SETVAL, 1)) == -1)
        {
            ERROR("masterSem #0 semctl");
        }
        /*setto la seconda a SO_USER_NUM+SO_NODES_NUM,
        ogni figlio ne prende 1 per dire al master quando hanno tutti finito*/
        if ((semctl(masterSemID, 1, SETVAL, SO_USERS_NUM + SO_NODES_NUM)) == -1)

        {
            ERROR("masterSem #1 semctl");
        }

        /*array condiviso con i PID degli user*/
        if ((userPIDsID = shmget(USERSK, SO_USERS_NUM * sizeof(int), NEWIPC)) == -1)
        {
            ERROR("shmemget userPIDs");
        }
        if ((userPIDs = (int *)shmat(userPIDsID, NULL, 0)) == (void *)-1)
        {
            ERROR("shmat userPIDs");
        }

        /*Libro Mastro*/
        if ((LMID = shmget(LEDGERK, sizeof(libroMastro), NEWIPC)) == -1)
        {
            ERROR("master shmemget ledger");
        }
        if ((ledger = (libroMastro *)shmat(LMID, NULL, 0)) == (void *)-1)
        {
            ERROR("shmat ledger");
        }
        ledger->startTime.tv_sec = -1;

        ledger->blockCounter = 0;
        ledger->deadUsers = 0;
    }
    /*genera user*/
    for (i = 0; i < SO_USERS_NUM; i++)
    {
        switch (childPID = fork())
        {
        case 0:
            execl("user", "newuser.c", NULL);
            break;

        default:
            /*ha figliato, salva il pid*/
            userPIDs[i] = childPID;
            if (i == SO_USERS_NUM - 1)
            {
                printf("MASTER: users birthed (%d - %d)\n", userPIDs[0], userPIDs[i]);
            }
            break;
        }
    }

    /*array condiviso con i PID dei nodi*/
    if ((nodePIDsID = shmget(NODESK, SO_NODES_NUM * sizeof(int), NEWIPC)) == -1)
    {
        ERROR("shmemget nodePIDs");
    }
    if ((nodePIDs = (int *)shmat(nodePIDsID, NULL, 0)) == (void *)-1)
    {
        ERROR("shmat nodePIDs");
    }

    /*array condiviso con i PID dei reader*/
    if ((readerPIDsID = shmget(READERSK, SO_NODES_NUM * sizeof(int), NEWIPC)) == -1)
    {
        ERROR("shmemget readerPIDs");
    }
    if ((readerPIDs = (int *)shmat(readerPIDsID, NULL, 0)) == (void *)-1)
    {
        ERROR("shmat readerPIDs");
    }

    /*genera nodi*/
    for (i = 0; i < SO_NODES_NUM; i++)
    {
        switch (childPID = fork())
        {
        case 0:
            /*siamo nel figlio*/
            execl("node", "newnode.c", NULL);
            break;

        default:
            /*ha figliato, salva il pid*/
            nodePIDs[i] = childPID;
            if (i == SO_NODES_NUM - 1)
            {
                /*tutti i figli sono nati*/
                printf("MASTER: nodes birthed (%d - %d)\n", nodePIDs[0], nodePIDs[i]);
            }
            break;
        }
    }
    sigemptyset(&mask);
    sa.sa_handler = masterSigHandler;
    sa.sa_mask = mask;
    sa.sa_flags = 0;
    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGALRM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = sigChldHandler;
    sigaction(SIGCHLD, &sa, NULL);

    /*dice ai figli di partire*/
    quickTake(masterSemID, 0);

    quickWait(masterSemID, 1);
    /*è qua quando N+U+NR sono tutti pronti*/
    clock_gettime(CLOCK_REALTIME, &(ledger->startTime));
    alarm(SO_SIM_SEC);

    finished = 0;
    printf("\nMASTER: SIMULATION STARTED\n");

    /* setta a 1 , servirà ai nodi per darsi i turni nell'inserire blocchi nel ledger*/
    if (semctl(masterSemID, 0, SETVAL, 1) == -1)
    {
        ERROR("semctl masterSem");
    }

    while (1)
    {
        /*stampa periodica*/
        sleepTime.tv_nsec = 0;
        sleepTime.tv_sec = 1;
        nanosleep(&sleepTime, NULL);

        printf("+ + + + + + + + + + + + + + + + + + + + + + + + +\n");
        printf("SIMULATION STATUS: %d/%d users alive // %d/%d blocks in ledger\n\n",
               SO_USERS_NUM - ledger->deadUsers,
               SO_USERS_NUM,
               ledger->blockCounter, SO_REGISTRY_SIZE);
        calculateBalances();
        printf("+ + + + + + + + + + + + + + + + + + + + + + + + +\n");
    }
}