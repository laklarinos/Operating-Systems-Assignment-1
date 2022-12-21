#include "includes.h"
#include "helpers.h"
// **To Add** should move this to a common file

int main(int argc, char *argv[])
{
    int key = (int)SHMKEY;
    int sgmt = atoi(argv[1]);
    int num_of_lines_per_segment = atoi(argv[2]);
    char *sem_name = argv[3];
    int num_of_requests = atoi(argv[4]);
    char *w_sgmt_sem_name = argv[5];
    char *r_sgmt_sem_name = argv[6];
    char *rw_mutex_name = argv[7];
    char *dummy_name = argv[8];
    int pid = getpid();
    srand(pid);
    char *file_name_out;
    char *pid_char;

    itoa((int)getpid(), &pid_char);
    file_name_out = malloc((strlen("output") + strlen(pid_char) + strlen(".txt") + 1) * sizeof(char));
    strcat(file_name_out, pid_char);
    strcat(file_name_out, ".txt");

    int requested_sgmt;
    int requested_line;
    int offset;
    FILE *fp_out = fopen(file_name_out, "w");
    char *array_of_sem_names[sgmt];
    char *tmp;
    char *line;
    sem_t *rw_mutex;
    sem_t *w_sgmt_sem;
    sem_t *r_sgmt_sem;
    sem_t *dummy;
    sem_t **array_of_sem = malloc(sgmt * sizeof(sem_t));
    void *shared_memory_void_ptr = (void *)0;
    struct sharedMem *shmem;
    int shmid;
    double elapsed_answer;
    double elapsed_request;
    struct timespec tstart_req = {0, 0}, tend_req = {0, 0};
    struct timespec tstart_ans = {0, 0}, tend_ans = {0, 0};

    /**************************** STORING SEMAPHORE NAMES ****************************/
    for (int i = 0; i < sgmt; i++)
    {
        itoa(i + 1, &tmp);
        array_of_sem_names[i] = malloc((strlen("sem_") + 1 + countDigits(i)) * sizeof(char));
        strcpy(array_of_sem_names[i], "sem_");
        strcat(array_of_sem_names[i], tmp);
        free(tmp);
    }

    /**************************** INITIALIZING SEMAPHORES ****************************/
    if ((rw_mutex = sem_open(rw_mutex_name, O_CREAT, SEM_PERMS, 1)) == SEM_FAILED)
    {
        fprintf(stderr, "sem_open() failed.  errno:%d\n", errno);
    }

    if ((w_sgmt_sem = sem_open(w_sgmt_sem_name, O_CREAT, SEM_PERMS, 0)) == SEM_FAILED)
    {
        fprintf(stderr, "sem_open() failed.  errno:%d\n", errno);
    }

    if ((r_sgmt_sem = sem_open(r_sgmt_sem_name, O_CREAT, SEM_PERMS, 0)) == SEM_FAILED)
    {
        fprintf(stderr, "sem_open() failed.  errno:%d\n", errno);
    }

    if ((dummy = sem_open(dummy_name, O_CREAT, SEM_PERMS, 0)) == SEM_FAILED)
    {
        fprintf(stderr, "sem_open() failed.  errno:%d\n", errno);
    }

    for (int i = 0; i < sgmt; i++)
    {
        if ((array_of_sem[i] = sem_open(array_of_sem_names[i], O_CREAT, SEM_PERMS, 1)) == SEM_FAILED)
        {
            fprintf(stderr, "sem_open() failed.  errno:%d\n", errno);
        }
    }

    /**************************** SHARED MEMORY ****************************/
    if ((shmid = shmget(SHMKEY, sizeof(sharedMem), PERMS)) < 0)
    {
        fprintf(stderr, "shmget failed.  errno:%d\n", errno);
        exit(1);
    }

    if ((shmem = (sharedMem *)shmat(shmid, (void *)0, 0)) == (sharedMem *)-1)
    {
        fprintf(stderr, "shmat failed.  errno:%d\n", errno);
        exit(1);
    }

    fprintf(fp_out, "<requested line, requested segment> | elapsed time of request | elapsed time of answer | line\n\n");

    /**************************** REQUESTS ****************************/
    for (int i = 0; i < num_of_requests; i++)
    {
        requested_sgmt = (rand() % (sgmt - 1)) + 1;
        requested_line = (rand() % (num_of_lines_per_segment - 1)) + 1;

        clock_gettime(CLOCK_MONOTONIC, &tstart_req);

        if (sem_wait(array_of_sem[requested_sgmt - 1]) < 0)
        {
            fprintf(stderr, "sem_wait() failed.  errno:%d\n", errno);
            exit(EXIT_FAILURE);
        }

        shmem->array_of_read_count[requested_sgmt - 1]++;

        if (shmem->array_of_read_count[requested_sgmt - 1] == 1 && requested_sgmt != shmem->current_sgmt)
        {
            if (sem_wait(w_sgmt_sem) < 0)
            {
                fprintf(stderr, "sem_wait() failed.  errno:%d\n", errno);
                exit(EXIT_FAILURE);
            }

            shmem->requested_sgmt = requested_sgmt;

            if (sem_post(r_sgmt_sem) < 0)
            {
                fprintf(stderr, "sem_post() failed.  errno:%d\n", errno);
                exit(EXIT_FAILURE);
            }

            if (sem_wait(rw_mutex) < 0)
            {
                fprintf(stderr, "sem_wait() failed.  errno:%d\n", errno);
                exit(EXIT_FAILURE);
            }

            if (sem_post(dummy) < 0)
            {
                fprintf(stderr, "sem_post() failed.  errno:%d\n", errno);
                exit(EXIT_FAILURE);
            }
        }

        if (sem_post(array_of_sem[requested_sgmt - 1]) < 0)
        {
            fprintf(stderr, "sem_wait() failed.  errno:%d\n", errno);
            exit(EXIT_FAILURE);
        }

        clock_gettime(CLOCK_MONOTONIC, &tstart_ans);
        line = malloc(MAX_LINE_LENGTH * sizeof(char));
        memcpy(line, shmem->buffer[requested_line - 1], MAX_LINE_LENGTH);
        clock_gettime(CLOCK_MONOTONIC, &tend_ans);
        shmem->requests++;
        printf("%d child, retrieved %d line from %d segment\n", getpid(), requested_line, requested_sgmt);
        fflush(stdout);
        usleep(20000);
        clock_gettime(CLOCK_MONOTONIC, &tend_req);

        fprintf(fp_out, "<%d, %d> | ", requested_sgmt, requested_line);
        elapsed_request = ((double)tend_req.tv_sec + 1.0e-9 * tend_req.tv_nsec) - ((double)tstart_req.tv_sec + 1.0e-9 * tstart_req.tv_nsec);
        elapsed_answer = ((double)tend_ans.tv_sec + 1.0e-9 * tend_ans.tv_nsec) - ((double)tstart_ans.tv_sec + 1.0e-9 * tstart_ans.tv_nsec);

        fprintf(fp_out, "%.5f sec| ", elapsed_request);
        fprintf(fp_out, "%.5f sec| ", elapsed_answer);
        fprintf(fp_out, "%s", line);
        free(line);

        if (sem_wait(array_of_sem[requested_sgmt - 1]) < 0)
        {
            fprintf(stderr, "sem_wait() failed.  errno:%d\n", errno);
            exit(EXIT_FAILURE);
        }

        shmem->array_of_read_count[requested_sgmt - 1]--;

        if (shmem->array_of_read_count[requested_sgmt - 1] == 0)
        {
            if (sem_post(rw_mutex) < 0)
            {
                fprintf(stderr, "sem_wait() failed.  errno:%d\n", errno);
                exit(EXIT_FAILURE);
            }
        }

        if (sem_post(array_of_sem[requested_sgmt - 1]) < 0)
        {
            fprintf(stderr, "sem_wait() failed.  errno:%d\n", errno);
            exit(EXIT_FAILURE);
        }
    }

    /**************************** CLOSING SEMAPHORES ****************************/
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

    if (sem_close(dummy) < 0)
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

    /**************************** DEALLOCATING MEMORY ****************************/
    for (int i = 0; i < sgmt; i++)
    {
        free(array_of_sem_names[i]);
    }
    free(array_of_sem);
    free(file_name_out);
    fclose(fp_out);
}