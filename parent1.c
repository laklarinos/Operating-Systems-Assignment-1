#include "includes.h"
#include "helpers.h"

int main(int argc, char *argv[])
{

    char *file_name;
    char *file_name_out;
    int sgmt;
    int num_of_child;
    int num_of_requests;
    pid_t wpid;
    int status = 0;
    int request_counter = 0;
    FILE *fp;
    FILE *fp_out;

    if (argc != 5)
    {
        printf("Please specify a file name, a segmentation number and a number of requests. %d\n", argc);
        exit(1);
        // *TO ADD* maybe put scanf
    }
    else
    {
        file_name = argv[1];
        file_name_out = "output.txt";
        sgmt = atoi(argv[2]);
        num_of_child = atoi(argv[3]);
        num_of_requests = atoi(argv[4]);
    }

    fp = fopen(file_name, "r");
    fp_out = fopen("output.txt", "w");

    if (fp == NULL)
    {
        printf("This file does not exist.\n");
        exit(1);
    }

    int num_lines = 0;
    char **array_of_txt_lines;
    segment **array_of_sgmt = malloc(sgmt * sizeof(segment *));
    int rem_lines;
    int shmid, semid;
    sharedMem *shmem;
    int num_of_lines_per_segment;
    int pid;
    int offset;
    char *tmp;
    char *w_sgmt_sem_name = "w_sgmt_sem";
    char *r_sgmt_sem_name = "r_sgmt_sem";
    char *sem_name = "sem_";
    char *rw_mutex_name = "rw_mutex";
    char *shmkey_char;
    char *shmkey2_char;
    char *shmkey3_char;
    char *sgmt_char;
    char *num_of_lines_per_segment_char;
    char *num_of_requests_char;

    sem_t **array_of_sem = malloc(sgmt * sizeof(sem_t));
    sem_t *r_sgmt_sem;
    sem_t *w_sgmt_sem;
    sem_t *rw_mutex;
    num_lines = countLines(fp);

    // to get pointer at beggining
    fseek(fp, 0, SEEK_SET);

    // create array of lines
    if (create_array_of_txt_lines(fp, &array_of_txt_lines, num_lines) == NULL)
    {
        printf("Create_array_of_txt_lines func collapsed.\n");
        exit(1);
    }

    // to get pointer at beggining
    fseek(fp, 0, SEEK_SET);

    // first decide how many arrays you will want, based on the number of sgmt
    if (sgmt > num_lines)
    {
        printf("Segmentation is greater than the number of lines.\n");
        exit(1);
    }
    else if (sgmt == 0)
    {
        sgmt = 1;
    }

    for (int i = 0; i < sgmt; i++)
    {
        array_of_sgmt[i] = malloc(sizeof(segment));
        array_of_sgmt[i]->array_of_lines = NULL;
        array_of_sgmt[i]->num_of_lines = 0;
    }

    // every segment grabs a line from the array, until there are no left
    rem_lines = num_lines;
    while (rem_lines > 0)
    {
        for (int i = 0; i < sgmt; i++)
        {
            int j = array_of_sgmt[i]->num_of_lines;
            if (rem_lines == 0)
                break;
            array_of_sgmt[i]->array_of_lines = realloc(array_of_sgmt[i]->array_of_lines, (j + 1) * sizeof(char **));
            array_of_sgmt[i]->array_of_lines[array_of_sgmt[i]->num_of_lines] = &array_of_txt_lines[num_lines - rem_lines];
            array_of_sgmt[i]->num_of_lines++;
            // printf("%s, %ld\n", *(array_of_sgmt[i]->array_of_lines[j]), strlen(*(array_of_sgmt[i]->array_of_lines[j])));
            rem_lines--;
        }
    }

    // the idea is that I will create an 1D char array containing the lines of each segmen
    // then I will pass this array through the shared memory
    num_of_lines_per_segment = num_lines / sgmt;

    if ((shmid = shmget(SHMKEY, sizeof(sharedMem) + num_of_lines_per_segment * MAX_LINE_LENGTH * sizeof(char *) + sgmt * sizeof(int), PERMS)) < 0)
    {
        fprintf(stderr, "shmget failed.  errno:%d\n", errno);
        exit(1);
    }
    printf("Creating shared memory with ID: %d\n", shmid);

    if ((shmem = (sharedMem *)shmat(shmid, (void *)0, 0)) == (sharedMem *)-1)
    {
        fprintf(stderr, "shmat failed.  errno:%d\n", errno);
        exit(1);
    }

    ////////////////////// for char** array ///////////////////////
    if ((shmid = shmget(SHMKEY2, num_of_lines_per_segment * MAX_LINE_LENGTH * sizeof(char *), PERMS)) < 0)
    {
        fprintf(stderr, "shget->buffer failed.  errno:%d\n", errno);
        exit(1);
    }

    shmem->buffer = (char **)shmat(shmid, (void *)0, 0);
    if (shmem->buffer == NULL)
    {
        fprintf(stderr, "shmat->buffer failed.  errno:%d\n", errno);
    }
    ////////////////////// for char** array ///////////////////////

    ////////////////////// for each entry of char** ///////////////////////
    // for (int i = 0; i < num_of_lines_per_segment; i++)
    // {
    //     if ((shmid = shmget((key_t)((int)SHMKEY4 + i), MAX_LINE_LENGTH * sizeof(char), PERMS)) < 0)
    //     {
    //         fprintf(stderr, "shget->buffer failed.  errno:%d\n", errno);
    //         exit(1);
    //     }

    //     shmem->buffer[i] = (char *)shmat(shmid, (void *)0, 0);
    //     if (shmem->buffer[i] == (char *)(-1))
    //     {
    //         fprintf(stderr, "shmat->buffer failed.  errno:%d\n", errno);
    //     }
    //     shmem->buffer[i][0] = '\0';
    // }
    ////////////////////// for each entry of char** ///////////////////////

    ////////////////////// for of int* ///////////////////////
    if ((shmid = shmget(SHMKEY3, sgmt * sizeof(int), PERMS)) < 0)
    {
        fprintf(stderr, "shmget->array_or_read_count failed.  errno:%d\n", errno);
        exit(1);
    }

    shmem->array_of_read_count = (int *)shmat(shmid, (void *)0, 0);
    if (shmem->array_of_read_count == (int *)(-1))
    {
        fprintf(stderr, "shmem->array_read_count failed.  errno:%d\n", errno);
    }
    ////////////////////// for of int* ///////////////////////

    printf("Attaching shared memory segment. \n");

    // initializing semaphores
    // beacuse we need multiple processes to gain access of the shared memory,
    // we cant use binary semaphores, thus
    // we need counting semaphores...
    char *array_of_sem_names[sgmt];

    for (int i = 0; i < sgmt; i++)
    {
        itoa(i + 1, &tmp);
        array_of_sem_names[i] = malloc((strlen("sem_") + 1 + countDigits(i)) * sizeof(char));
        strcpy(array_of_sem_names[i], "sem_");
        strcat(array_of_sem_names[i], tmp);
        free(tmp);
    }

    for (int i = 0; i < sgmt; i++)
    {
        if ((array_of_sem[i] = sem_open(array_of_sem_names[i], O_CREAT, SEM_PERMS, 1)) == SEM_FAILED)
        {
            fprintf(stderr, "sem_open() failed.  errno:%d\n", errno);
        }
    }

    if ((w_sgmt_sem = sem_open(w_sgmt_sem_name, O_CREAT, SEM_PERMS, 0)) == SEM_FAILED)
    {
        fprintf(stderr, "sem_open() failed.  errno:%d\n", errno);
    }

    if ((r_sgmt_sem = sem_open(r_sgmt_sem_name, O_CREAT, SEM_PERMS, 0)) == SEM_FAILED)
    {
        fprintf(stderr, "sem_open() failed.  errno:%d\n", errno);
    }

    if ((rw_mutex = sem_open(rw_mutex_name, O_CREAT, SEM_PERMS, 1)) == SEM_FAILED)
    {
        fprintf(stderr, "sem_open() failed.  errno:%d\n", errno);
    }

    itoa(sgmt, &sgmt_char);
    itoa(num_of_lines_per_segment, &num_of_lines_per_segment_char);
    itoa((int)SHMKEY, &shmkey_char);
    itoa((int)SHMKEY2, &shmkey2_char);
    itoa((int)SHMKEY3, &shmkey3_char);
    itoa(num_of_requests, &num_of_requests_char);

    for (int i = 0; i < num_of_child; i++)
    {
        pid = fork();
        if (pid < 0)
        {
            printf("Error forking child.\n");
            exit(1);
        }
        else if (pid == 0)
        {
            // children code
            // call execv to run client program
            char *args_to_send[] = {"./child",
                                    sgmt_char,
                                    num_of_lines_per_segment_char,
                                    sem_name,
                                    num_of_requests_char,
                                    w_sgmt_sem_name,
                                    r_sgmt_sem_name,
                                    rw_mutex_name,
                                    file_name_out,
                                    NULL};
            if (execv(args_to_send[0], args_to_send) < 0)
            {
                perror("Execv");
            }
        }
    }

    int requested_sgmt;
    int array_of_requests[num_of_requests * num_of_child];
    int current_sgmt = -1;
    // shmem->array_of_read_count = malloc(sgmt * sizeof(int));
    for (int i = 0; i < sgmt; i++)
    {
        shmem->array_of_read_count[i] = 0;
    }
    shmem->requested_sgmt = -1;

    while (request_counter < num_of_child * num_of_requests)
    {
        // parent increases semaphore and child is unblocked to send request
        if (sem_wait(rw_mutex) < 0)
        {
            fprintf(stderr, "sem_wait() failed.  errno:%d\n", errno);
            exit(EXIT_FAILURE);
        }

        printf("1\n");
        if (sem_post(w_sgmt_sem) < 0)
        {
            fprintf(stderr, "sem_post() failed.  errno:%d\n", errno);
            exit(EXIT_FAILURE);
        }

        // parent should wait before reading
        if (sem_wait(r_sgmt_sem) < 0)
        {
            fprintf(stderr, "sem_post() failed.  errno:%d\n", errno);
            exit(EXIT_FAILURE);
        }

        // parent can now read the request
        requested_sgmt = shmem->requested_sgmt;
        // meanwhile child is waiting for parent to put segment to shared mem

        if (shmem->requested_sgmt != current_sgmt)
        {
            printf("FIRST TIME IN\n");
            // that means, it is the first segment to put
            // putting segment to shared buffer

            for (int i = 0; i < array_of_sgmt[requested_sgmt - 1]->num_of_lines; i++)
            {
                char **temp = array_of_sgmt[requested_sgmt - 1]->array_of_lines[i];
                printf("%ld strlen temp\n", strlen(*temp));
                printf("%ld strlen buffer\n", strlen(shmem->buffer[i]));
                printf("%d requested\n", requested_sgmt);
                memcpy(shmem->buffer[i], *(array_of_sgmt[requested_sgmt - 1]->array_of_lines[i]), MAX_LINE_LENGTH);
            }

            current_sgmt = requested_sgmt;

            if (sem_post(rw_mutex) < 0)
            {
                fprintf(stderr, "sem_wait() failed.  errno:%d\n", errno);
                exit(EXIT_FAILURE);
            }
        }
        printf("NOPE\n");

        request_counter++;
    }

    while ((wpid = wait(&status)) > 0)
        ;
}