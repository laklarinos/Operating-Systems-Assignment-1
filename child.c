#include "includes.h"
#include "helpers.h"
// **To Add** should move this to a common file

int main(int argc, char *argv[])
{
    srand(time(NULL) ^ (getpid() << 16));

    int key = (int)SHMKEY;
    int sgmt = atoi(argv[1]);
    int num_of_lines_per_segment = atoi(argv[2]);
    char *sem_name = argv[3];
    int num_of_requests = atoi(argv[4]);
    char *w_sgmt_sem_name = argv[5];
    char *r_sgmt_sem_name = argv[6];
    char *rw_mutex_name = argv[7];

    char *file_name_out;
    char *pid_char;
    itoa((int)getpid(), &pid_char);
    file_name_out = malloc((strlen("output") + strlen(pid_char) + strlen(".txt") + 1) * sizeof(char));
    strcat(file_name_out, pid_char);
    strcat(file_name_out, ".txt");

    printf("%s\n", file_name_out);

    char *w_on_file_out_sem_name = "w_on_file_out";
    int requested_sgmt;
    int requested_line;
    int offset;
    FILE *fp_out = fopen(file_name_out, "w");
    char *array_of_sem_names[sgmt];
    char *tmp;
    sem_t *rw_mutex;
    sem_t *w_sgmt_sem;
    sem_t *r_sgmt_sem;
    sem_t **array_of_sem = malloc(sgmt * sizeof(sem_t));
    sem_t *w_on_file_out_sem;
    void *shared_memory_void_ptr = (void *)0;
    struct sharedMem *shmem;
    int shmid;

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

    if ((w_on_file_out_sem = sem_open(w_on_file_out_sem_name, O_CREAT, SEM_PERMS, 1)) == SEM_FAILED)
    {
        fprintf(stderr, "sem_open() failed.  errno:%d\n", errno);
    }

    if ((r_sgmt_sem = sem_open(r_sgmt_sem_name, O_CREAT, SEM_PERMS, 0)) == SEM_FAILED)
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

    /**************************** REQUESTS ****************************/
    for (int i = 0; i < num_of_requests; i++)
    {
        if (sem_wait(w_sgmt_sem) < 0)
        {
            fprintf(stderr, "sem_wait() failed.  errno:%d\n", errno);
            exit(EXIT_FAILURE);
        }

        requested_sgmt = (rand() % (sgmt - 1 + 1)) + 1;
        requested_line = (rand() % (num_of_lines_per_segment - 1)) + 1;
        offset = MAX_LINE_LENGTH * (requested_line - 1);

        // printf("Child %d asked for %d segment\n", getpid(), requested_sgmt);
        shmem->requested_sgmt = requested_sgmt;

        // child done writing, tells parent to proceed to read
        if (sem_post(r_sgmt_sem) < 0)
        {
            fprintf(stderr, "sem_post() failed.  errno:%d\n", errno);
            exit(EXIT_FAILURE);
        }

        if (sem_wait(array_of_sem[requested_sgmt - 1]) < 0)
        {
            fprintf(stderr, "sem_wait() failed.  errno:%d\n", errno);
            exit(EXIT_FAILURE);
        }

        shmem->array_of_read_count[requested_sgmt - 1]++;

        if (shmem->array_of_read_count[requested_sgmt - 1] == 1)
        {
            // printf("NOT PROCEED: Child %d asked for %d segment\n", getpid(), requested_sgmt);
            if (sem_wait(rw_mutex) < 0)
            {
                fprintf(stderr, "sem_wait() failed.  errno:%d\n", errno);
                exit(EXIT_FAILURE);
            }
            // printf("Child %d asked for %d segment\n", getpid(), requested_sgmt);
        }

        // printf("PROCEEDS: Child %d asked for %d segment\n", getpid(), requested_sgmt);
        if (sem_post(array_of_sem[requested_sgmt - 1]) < 0)
        {
            fprintf(stderr, "sem_wait() failed.  errno:%d\n", errno);
            exit(EXIT_FAILURE);
        }

        // here grab the line you want

        char *line = malloc(MAX_LINE_LENGTH * sizeof(char));
        memcpy(line, shmem->buffer[requested_line - 1], MAX_LINE_LENGTH);
        usleep(20);

        fprintf(fp_out, "%d %d %d:", requested_sgmt - 1, requested_line, getpid());
        fprintf(fp_out, "%s", line);

        if (sem_wait(array_of_sem[requested_sgmt - 1]) < 0)
        {
            fprintf(stderr, "sem_wait() failed.  errno:%d\n", errno);
            exit(EXIT_FAILURE);
        }

        shmem->array_of_read_count[requested_sgmt - 1]--;
        printf("Child %d satisfied request for %d segment and line %d\n", getpid(), requested_sgmt, requested_line);

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
    fclose(fp_out);
}