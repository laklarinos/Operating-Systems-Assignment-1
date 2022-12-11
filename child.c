#include "includes.h"
#include "helpers.h"

// **To Add** should move this to a common file

int main(int argc, char *argv[])
{
    // char *args_to_send[] = {"./child", shmkey_char, sgmt_char, num_of_lines_per_segment_char, MAX_LINE_LENGTH_char, NULL};
    int key = atoi(argv[1]);
    int sgmt = atoi(argv[2]);
    int number_of_lines_per_segment = atoi(argv[3]);
    char *sem_name = argv[4];

    char *array_of_sem_names[sgmt];
    char *tmp;
    for (int i = 0; i < sgmt; i++)
    {
        itoa(i + 1, &tmp);
        array_of_sem_names[i] = malloc((strlen("sem_") + 1 + countDigits(i)) * sizeof(char));
        strcpy(array_of_sem_names[i], "sem_");
        strcat(array_of_sem_names[i], tmp);
        free(tmp);
    }

    sem_t **semaphore_array = malloc(sgmt * sizeof(sem_t));
    for (int i = 0; i < sgmt; i++)
    {
        if ((semaphore_array[i] = sem_open(array_of_sem_names[i], O_CREAT, SEM_PERMS, 0)) == SEM_FAILED)
        {
            fprintf(stderr, "sem_open() failed.  errno:%d\n", errno);
        }
    }

    void *sharedMemory = (void *)0;
    struct sharedMem *sharedStuff;
    char buffer[BUFSIZ];
    int shmid;

    shmid = shmget((key_t)key, number_of_lines_per_segment * MAX_LINE_LENGTH * sizeof(char), 0666 | IPC_CREAT);

    if (shmid == -1)
    {
        perror("shmget failed\n");
        exit(EXIT_FAILURE);
    }

    sharedMemory = shmat(shmid, (void *)0, 0);

    if (sharedMemory == (void *)-1)
    {
        perror("shmat failed\n");
        exit(EXIT_FAILURE);
    }

    
}