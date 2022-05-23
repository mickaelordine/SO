#include "include.h"

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

int utente(){
    int shm_iduser,i,msgid,shm_keys,shm_idnodes; /*chiave per la msgQueue*/
    int *arrayWithUserId, *arrayWithkeys, *arrayWithNodesId;
    key_t msg_id;
    int SO_USERS_NUM,SO_NODES_NUM,SO_BUDGET_INIT,SO_SIM_SEC;


    /*assegnamento dei valori ad alcune variabili
    definite in tempo di esecuzione*/
    SO_USERS_NUM = get_const("SO_USERS_NUM");
    SO_BUDGET_INIT = get_const("SO_BUDGET_INIT");
    SO_NODES_NUM = get_const("SO_NODES_NUM");
    SO_SIM_SEC = get_const("SO_SIM_SEC");

    /*ci colleghiamo all'array di chiavi*/
    shm_keys = shmget(7777,IPC_NUM*sizeof(key_t), IPC_CREAT|0666);
    arrayWithkeys = (int*)shmat(shm_keys,NULL,0);

    /*mi collego all'array dei figli*/
    shm_iduser = shmget(arrayWithkeys[0], get_const("SO_USERS_NUM")*sizeof(int), IPC_CREAT|0666);
    arrayWithUserId = (int*)shmat(shm_iduser,NULL,0);

    /*mi collego all'array dei nodi*/
    shm_idnodes = shmget(arrayWithkeys[1], get_const("SO_NODES_NUM")*sizeof(int), IPC_CREAT|0666);
    arrayWithNodesId =  (int*)shmat(shm_idnodes,NULL,0);


    printf("sono il figlio %d e vedo: \n", getpid());
    printf("gli utenti : \n");
    for(i = 0; i<get_const("SO_USERS_NUM"); i++){
        printf("%d, \n",arrayWithUserId[i]);
    }

    

}