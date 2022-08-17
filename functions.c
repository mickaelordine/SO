#include "head.h"

int getReward(transaction t)
{
    double rew;
    int ret;
    rew = t.quantity * ((double)t.reward) / 100;
    ret = ROUND(rew);
    return (ret < 1) ? (1) : (ret);
}

int descending(const void *a, const void *b)
{
    int int_a = *((int *)a);
    int int_b = *((int *)b);

    if (int_a == int_b)
        return 0;
    else if (int_a < int_b)
        return 1;
    else
        return -1;
}


int indexOf(int searched, int *arr, int size)
{
    int i;
    for (i = 0; i < size; i++)
    {
        if (arr[i] == searched || arr[i] == -searched)
        {
            return i;
        }
    }
    return -1;
}

int intContains(int *arr, int size, int val)
{
    int i = 0;
    for (i; i < size; i++)
    {
        if (arr[i] == val || arr[i] == -val)
            return 1;
    }
    return 0;
}

int quickFree(int semid, int index)
{
    struct sembuf op;
    op.sem_op = 1;
    op.sem_num = index;
    op.sem_flg = 0;
    return semop(semid, &op, 1);
}
int quickTake(int semid, int index)
{
    struct sembuf op;
    op.sem_op = -1;
    op.sem_num = index;
    op.sem_flg = 0;
    return semop(semid, &op, 1);
}
int quickWait(int semid, int index)
{
    struct sembuf op;
    op.sem_op = 0;
    op.sem_num = index;
    op.sem_flg = 0;
    return semop(semid, &op, 1);
}

int randMaxEscl(int max)
{
    srandom(getpid() * max);
    return random() % max;
}

int randRangeIncl(int min, int max)
{
    srandom(getpid() + min * max);
    return random() % (max - min + 1) + min;
}
