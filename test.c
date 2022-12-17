#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define DEFAULT_PROTOCOL 0
typedef struct struarr
{
    int a;
    int b;
} struarr;
typedef struct stru1
{
    int index;
    struarr *arrstruct;
} stru1;

int main()
{
    stru1 *structptr;
    int i = 0;
    int j = 0;
    key_t key;
    key_t key2;
    int shmid;

    key = 5454;
    key2 = 5555;

    shmid = shmget(key, sizeof(stru1), 0666 | IPC_CREAT);
    if (shmid < 0)
    {
        perror("err");
    }
    structptr = (stru1 *)shmat(shmid, (void *)0, 0);
    if (structptr == (stru1 *)(-1))
        perror("shmat1");

    if (fork() == 0)
    {
        int b = 0;

        structptr->index = 4;
        shmid = shmget(key2, (structptr->index * sizeof(struarr)), 0666 | IPC_CREAT);
        if (shmid < 0)
        {
            perror("err2");
        }
        structptr->arrstruct = (struarr *)shmat(shmid, (void *)0, 0);
        if (structptr->arrstruct == (struarr *)(-1))
            perror("shmat11");

        while (i < structptr->index)
        {

            structptr->arrstruct[i].a = b;
            b++;
            structptr->arrstruct[i].b = b;
            b++;
            i++;
        }
        j = 0;
        while (j < structptr->index)
        {
            printf("a:%d\tb:%d\n", structptr->arrstruct[j].a, structptr->arrstruct[j].b);
            j++;
        }

        i = 0;
        while (i < 5)
        {
            i++;
            sleep(1);
            scanf("%d", &b);
            j = 0;
            while (j < structptr->index)
            {
                sleep(1);
                structptr->arrstruct[j].a = b;
                b++;
                structptr->arrstruct[j].b = b;
                b++;
                j++;
            }
        }
        shmdt(structptr);
        shmctl(shmid, IPC_RMID, 0);
    }

    sleep(2);
    shmid = shmget(key2, (structptr->index * sizeof(struarr)), 0666 | IPC_CREAT);
    if (shmid < 0)
    {
        perror("err2");
    }
    structptr->arrstruct = (struarr *)shmat(shmid, (void *)0, 0);
    if (structptr->arrstruct == (struarr *)(-1))
        perror("shmat11");

    i = 0;
    while (i < 30)
    {
        i++;
        sleep(1);

        j = 0;
        while (j < structptr->index)
        {
            printf("a:%d\tb:%d\n", structptr->arrstruct[j].a, structptr->arrstruct[j].b);
            j++;
        }
    }
    shmdt(structptr);
    shmctl(shmid, IPC_RMID, 0);
    return 0;
}