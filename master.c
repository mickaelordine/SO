#include "include.h"

/*shortcut*/
#define NEWIPC (IPC_CREAT | IPC_EXCL | 00666)
#define GETIPC (IPC_CREAT | 00666)
#define READIPC (IPC_CREAT | 00600)
#define NODEREWARD -1
#define UTON 1
#define DEADUSER -1
#define SEMSIZE 2



int main(int argc, char* argv[]){
    /*creo un array contenenti gli msgid delle msgqueue, con queste key gli utenti sapranno a quale key attaccarsi con key_t idnode = arrayWithNodesId[rans%SO_NODES_NUM] 
    per vedere a quale msgqueue attaccarsi*/
    int msgid,shm_iduser,i,shm_idnodes,shm_keys,masterSem,res,syncID;
    int SO_USERS_NUM,SO_NODES_NUM,SO_BUDGET_INIT,SO_SIM_SEC,KEY_ID;
    int *arrayWithUserId,*arrayWithNodesId, *utenti_morti,*utenti_vivi,*sync;
    key_t msg_id;
    key_t * key;
    pid_t pid;


    /*assegnamento dei valori ad alcune variabili
    definite in tempo di esecuzione*/
    SO_USERS_NUM = get_const("SO_USERS_NUM");
    SO_BUDGET_INIT = get_const("SO_BUDGET_INIT");
    SO_NODES_NUM = get_const("SO_NODES_NUM");
    SO_SIM_SEC = get_const("SO_SIM_SEC");
    KEY_ID = get_const("KEY_ID");


    /*
    KEYS IPC:
    [0]->array con pid degli utenti
    [1]->array con pid dei nodi
    [2]->libro mastro
    [3]->message queue
    [4]->semaforo mastro
    [5]->sync id
    */


    /*id per le chiavi*/
    shm_keys = shmget(KEY_ID,IPC_NUM*sizeof(key_t), IPC_CREAT|0666);

    key = (int*)shmat(shm_keys,NULL,0);
    /*assegnamento dei valori alle chiavi con ftok*/
    for(i = 0;i<IPC_NUM;i++) {
    key[i] = ftok("prog",(int) i);
    }
    printf("CHIAVI CREATE!\n");


    
    /*creo un array con  tutti gli id dei figli*/
    shm_iduser = shmget(key[0], get_const("SO_USERS_NUM")*sizeof(int), IPC_CREAT|0666);

    /*creo un unica msgqueu e la scelta del nodo è uguale al msgtype*/
    msgid = msgget(key[3], 0666 | IPC_CREAT);

    /*creo un array con tutti gli id dei nodi*/
    shm_idnodes= shmget(key[1], get_const("SO_NODES_NUM")*sizeof(int),IPC_CREAT|0666);

    /*creazione semaforo
    1a cella per l'inizializzazione N+U, poi per far partire i N
    1a cella per far partire gli U dopo i N*/
    masterSem = semget(key[4], SEMSIZE, IPC_CREAT | IPC_EXCL | 0666);
    if(masterSem==-1){
        printf("\n - - M: errore in masterSem semget - - ");
        return 1;
    }

    /*inizializzazione semaforo*/
    res = semctl(masterSem, 0, SETVAL, 1);
    printf("\nsetto masterSem #0 a 1: val = %d", semctl(masterSem, 0, GETVAL));
    if(res==-1){
        printf("\n - - M: errore in semctl[0] - - ");
        return 1;
    }
    
    res = semctl(masterSem, 1, SETVAL, 1);
    printf("\nsetto masterSem #1 a 1: val = %d", semctl(masterSem, 0, GETVAL));

    /*byte di sync*/
    syncID = shmget(key[5], sizeof(int), IPC_CREAT | IPC_EXCL | 0666);
    sync = (int*)shmat(syncID, NULL, 0);

    /*creazione users*/
    quickTake(masterSem, 0);
    *sync = 0;
    for(i=0;i<get_const("SO_USERS_NUM");i++){
        switch (pid=fork())
        {
        case 0: /*figlio*/
            arrayWithUserId = (int*)shmat(shm_iduser,NULL,0);
            arrayWithUserId[i] = getpid();
            utente();
            break;

        case 1: /*padre*/
            return 1;
            break;
        
         default:
            return -1;
            break;
        }
    }


    /*attesa fine users*/
    printf("attesa fine creazione users \n");
    while(*sync != SO_USERS_NUM){
    }
    printf("USERS CREATI \n");
    /*attesa fine users*/
    dprintf(2,"\nM: all usrs rdy, sync= %d", *sync);



    /*creazione dei nodi*/
    /*for(i=0;i<get_const("SO_NODES_NUM");i++){
        switch (pid=fork())
        {
        case 0: 
            arrayWithNodesId = (int*)shmat(shm_iduser,NULL,0);
            arrayWithNodesId[i] = getpid();
            
            break;


        case 1: 
            exit(EXIT_SUCCESS);
            break;

         default:
            exit(EXIT_FAILURE);
            break;
        }
    }*/
}