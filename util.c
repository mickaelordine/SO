#include "include.h"


int get_const(char * c) {
    int num;
    char * nome_const;
    FILE * f= fopen("const.txt","r");
    nome_const=malloc(sizeof(char)*30);

    while(fscanf(f,"%s %i",nome_const,&num) != EOF)
    {
        if(strcmp(nome_const,c)==0)
        {
            fclose(f);
            free(nome_const);
            return num;
        }
    }

    fclose(f);
    free(nome_const);
    return -1;
}


/********************************************************/


/*parte delle funzioni dei semafori*/

struct sembuf newSemOp(int index, int op, int flags){
    struct sembuf res;
    res.sem_flg = flags;
    res.sem_num = index;
    res.sem_op = op;
    return res;
}

struct sembuf quickOp(int index, const char * shortcut){
    if(!strcmp(shortcut, "take")){
        return newSemOp(index,-1,0);
    } else if(!strcmp(shortcut, "free")){
        return newSemOp(index,1,0);
    } else if(!strcmp(shortcut, "wait")){
        return newSemOp(index, 0, 0);
    } else {
        printf("unrecognised shortcut - meaningless semop(0,0,0) returned");
        return newSemOp(0,0,0);
    }
}

int quickFree(int semid, int index){
    struct sembuf op = quickOp(index, "free");
    return semop(semid, &op, 1);
}
int quickTake(int semid, int index){
    struct sembuf op = quickOp(index, "take");
    return semop(semid, &op, 1);
}
int quickWait(int semid, int index){
    struct sembuf op = quickOp(index, "wait");
    return semop(semid, &op, 1);
}
/*parte delle funzioni dei semafori*/


/********************************************************/

/*funzioni utenti*/

/*funzione per scegliere l'utente*/
int getDest(int * dests){
    int res = dests[rand()%get_const("SO_USERS_NUM")];
    return (res==getpid() || res==-1) ? getDest(dests) : res; 
}


/*per non mandare soldi a me stesso*/
int checkIsNotMe(int* arrayWithUserId){
    int userChose =arrayWithUserId[rand()%get_const("SO_USERS_NUM")];
    if(userChose == getpid()){
    return checkIsNotMe(arrayWithUserId);
    }
    else return userChose;
}

/*funzioni utenti*/


/********************************************************/
