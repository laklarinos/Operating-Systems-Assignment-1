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
    }
    else
    {
        file_name = argv[1];
        sgmt = atoi(argv[2]);
        num_of_child = atoi(argv[3]);
        num_of_requests = atoi(argv[4]);
    }

    fp = fopen(file_name, "r");
    if (fp == NULL)
    {
        printf("This file does not exist.\n");
        exit(1);
    }

    int num_lines = 0;
    int prob;
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
    char *dummy_name = "dummy";
    char *shmkey_char;
    char *shmkey2_char;
    char *shmkey3_char;
    char *sgmt_char;
    char *num_of_lines_per_segment_char;
    char *num_of_requests_char;
    char *array_of_sem_names[sgmt];

    sem_t **array_of_sem = malloc(sgmt * sizeof(sem_t));
    sem_t *r_sgmt_sem;
    sem_t *w_sgmt_sem;
    sem_t *rw_mutex;
    sem_t *dummy;
    num_lines = countLines(fp);

    struct timespec ts;

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
            rem_lines--;
        }
    }

    /**************************** SHARED MEMORY ****************************/
    num_of_lines_per_segment = num_lines / sgmt;

    if ((shmid = shmget(SHMKEY, sizeof(sharedMem) + sizeof(char[num_of_lines_per_segment][MAX_LINE_LENGTH]), PERMS)) < 0)
    {
        fprintf(stderr, "shmget failed.  errno:%d\n", errno);
        exit(1);
    }

    if ((shmem = (sharedMem *)shmat(shmid, (void *)0, 0)) == (sharedMem *)-1)
    {
        fprintf(stderr, "shmat failed.  errno:%d\n", errno);
        exit(1);
    }

    /**************************** INITIALIZING SEMAPHORES ****************************/
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

    if ((dummy = sem_open(dummy_name, O_CREAT, SEM_PERMS, 0)) == SEM_FAILED)
    {
        fprintf(stderr, "sem_open() failed.  errno:%d\n", errno);
    }

    /**************************** PREPROCESSING BEFORE EXEC ****************************/
    itoa(sgmt, &sgmt_char);
    itoa(num_of_lines_per_segment, &num_of_lines_per_segment_char);
    itoa((int)SHMKEY, &shmkey_char);
    itoa(num_of_requests, &num_of_requests_char);

    int requested_sgmt;
    int array_of_requests[num_of_requests * num_of_child];
    int current_sgmt = -1;
    int prev_current_sgmt = -1;

    for (int i = 0; i < sgmt; i++)
    {
        shmem->array_of_read_count[i] = 0;
    }
    shmem->requested_sgmt = -1;
    shmem->current_sgmt = -1;
    shmem->requests = 0;

    /**************************** GIVE BIRTH  ****************************/
    for (int i = 0; i < num_of_child; i++)
    {
        pid = fork();
        if (pid < 0)
        {
            fprintf(stderr, "fork failed.  errno:%d\n", errno);
            exit(1);
        }
        else if (pid == 0)
        {

            // children code
            char *args_to_send[] = {"./child",
                                    sgmt_char,
                                    num_of_lines_per_segment_char,
                                    sem_name,
                                    num_of_requests_char,
                                    w_sgmt_sem_name,
                                    r_sgmt_sem_name,
                                    rw_mutex_name,
                                    dummy_name,
                                    NULL};
            if (execv(args_to_send[0], args_to_send) < 0)
            {
                perror("Execv");
            }
        }
    }

    if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
    {
        fprintf(stderr, "clock_gettime() failed. errno:%d", errno);
    }

    ts.tv_sec += 12;

    /**************************** HANDLE REQUESTS ****************************/
    while (shmem->requests <= num_of_child * num_of_requests)
    {
        if (sem_wait(rw_mutex) < 0)
        {
            fprintf(stderr, "sem_wait() failed.  errno:%d\n", errno);
            exit(EXIT_FAILURE);
        }

        prob = (rand() % (10 - 1)) + 1;
        if (prob > 7 || shmem->current_sgmt == -1)
        {
            // switch segment, probability higher than 7
            // or
            // current segment is -1 which means it is the first ever
            // segment to be loaded
            if (sem_post(w_sgmt_sem) < 0)
            {
                fprintf(stderr, "sem_post() failed.  errno:%d\n", errno);
                exit(EXIT_FAILURE);
            }

            if (sem_timedwait(r_sgmt_sem, &ts) < 0)
            {
                if (errno != ETIMEDOUT)
                {
                    fprintf(stderr, "sem_post() failed.  errno:%d\n", errno);
                    exit(EXIT_FAILURE);
                }
                else
                {
                    printf("Is there anybody?\n");
                    break;
                }
            }

            requested_sgmt = shmem->requested_sgmt;

            for (int i = 0; i < array_of_sgmt[shmem->requested_sgmt - 1]->num_of_lines; i++)
            {
                shmem->buffer[i][0] = '\0';
                memcpy(shmem->buffer[i], *(array_of_sgmt[shmem->requested_sgmt - 1]->array_of_lines[i]), MAX_LINE_LENGTH);
            }

            shmem->current_sgmt = requested_sgmt;
            prev_current_sgmt = shmem->current_sgmt;
            //}
            if (sem_post(rw_mutex) < 0)
            {
                fprintf(stderr, "sem_wait() failed.  errno:%d\n", errno);
                exit(EXIT_FAILURE);
            }

            if (sem_wait(dummy) < 0)
            {
                fprintf(stderr, "sem_wait() failed.  errno:%d\n", errno);
                exit(EXIT_FAILURE);
            }
        }
        else if (prob <= 7 && shmem->array_of_read_count[shmem->current_sgmt - 1] == 0)
        {
            // prob <= 7 means we do not switch the segment
            // while checking if there is no reader still on queue
                // if there is none we have to switch the segment
            if (sem_post(w_sgmt_sem) < 0)
            {
                fprintf(stderr, "sem_post() failed.  errno:%d\n", errno);
                exit(EXIT_FAILURE);
            }

            if (sem_timedwait(r_sgmt_sem, &ts) < 0)
            {
                if (errno != ETIMEDOUT)
                {
                    fprintf(stderr, "sem_post() failed.  errno:%d\n", errno);
                    exit(EXIT_FAILURE);
                }
                else
                {
                    printf("Is there anybody?\n");
                    break;
                }
            }

            requested_sgmt = shmem->requested_sgmt;

            // load segment
            for (int i = 0; i < array_of_sgmt[shmem->requested_sgmt - 1]->num_of_lines; i++)
            {
                shmem->buffer[i][0] = '\0';
                memcpy(shmem->buffer[i], *(array_of_sgmt[shmem->requested_sgmt - 1]->array_of_lines[i]), MAX_LINE_LENGTH);
            }

            shmem->current_sgmt = requested_sgmt;
            prev_current_sgmt = shmem->current_sgmt;
            
            if (sem_post(rw_mutex) < 0)
            {
                fprintf(stderr, "sem_wait() failed.  errno:%d\n", errno);
                exit(EXIT_FAILURE);
            }

            if (sem_wait(dummy) < 0)
            {
                fprintf(stderr, "sem_wait() failed.  errno:%d\n", errno);
                exit(EXIT_FAILURE);
            }
        }
    }

    /**************************** CLOSING SEMAPHORES ****************************/
    while ((wpid = wait(&status)) > 0)
        ;

    if (sem_close(r_sgmt_sem) < 0)
    {
        perror("sem_close(3) 1 failed");
        exit(EXIT_FAILURE);
    }

    if (sem_close(w_sgmt_sem) < 0)
    {
        perror("sem_close(3) 2 failed");
        exit(EXIT_FAILURE);
    }

    if (sem_close(rw_mutex) < 0)
    {
        perror("sem_close(3) 3 failed");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < sgmt; i++)
    {
        if (sem_close(array_of_sem[i]) < 0)
        {
            perror("sem_close(3) 3 failed");
            exit(EXIT_FAILURE);
        }
    }

    if (sem_close(dummy) < 0)
    {
        perror("sem_close(3) 3 failed");
        exit(EXIT_FAILURE);
    }

    /**************************** UNLINKING SEMAPHORES ****************************/
    if (sem_unlink(r_sgmt_sem_name) < 0)
    {
        perror("sem_close(3) 1 failed");
        exit(EXIT_FAILURE);
    }

    if (sem_unlink(w_sgmt_sem_name) < 0)
    {
        perror("sem_close(3) 2 failed");
        exit(EXIT_FAILURE);
    }

    if (sem_unlink(rw_mutex_name) < 0)
    {
        perror("sem_close(3) 3 failed");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < sgmt; i++)
    {
        if (sem_unlink(array_of_sem_names[i]) < 0)
        {
            perror("sem_close(3) 3 failed");
            exit(EXIT_FAILURE);
        }
    }

    if (sem_unlink(dummy_name) < 0)
    {
        perror("sem_close(3) 3 failed");
        exit(EXIT_FAILURE);
    }

    /**************************** CLOSING SHARED MEMORY ****************************/
    if (shmdt(shmem) == -1)
    {
        perror("SharedMem dt");
        exit(EXIT_FAILURE);
    }
    if (shmctl(shmid, IPC_RMID, 0) == -1)
    {
        perror("ShmCtl");
        exit(EXIT_FAILURE);
    }

    /**************************** DEALLOCATING MEMORY ****************************/
    for (int i = 0; i < sgmt; i++)
    {
        free(array_of_sgmt[i]->array_of_lines);
        free(array_of_sgmt[i]);
    }

    free(array_of_sgmt);

    for (int i = 0; i < sgmt; i++)
    {
        free(array_of_sem_names[i]);
    }
    free(array_of_sem);

    for (int i = 0; i < num_lines; i++)
    {
        free(array_of_txt_lines[i]);
    }
    free(array_of_txt_lines);
}