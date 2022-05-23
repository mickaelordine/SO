#include "include.h"

int main(){
    int k,utenti;

    k=5; //chiave per l'array di uetnti
    utenti = get_const("SO_USERS_NUM");
    int* arrayWithUserId;
    int shm_iduser = shmget(k, get_const("SO_USERS_NUM")*sizeof(int), IPC_CREAT|0666);
    arrayWithUserId = (int*)shmat(shm_iduser,NULL,0);

    for(int i = 0; i < get_const("SO_USERS_NUM"); i++){
        printf("arrayWithUserId[%d] : %d\n", i,arrayWithUserId[i]);
    }

}