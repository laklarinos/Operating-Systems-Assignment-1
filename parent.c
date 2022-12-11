#include "includes.h"
#include "helpers.h"

int main(int argc, char *argv[])
{
    char *file_name;
    // number of segments
    int sgmt;
    int num_of_childer;
    if (argc < 3)
    {
        printf("Please specify a file name and a segmentation number. %d\n", argc);
        exit(1);
        // *TO ADD* maybe put scanf
    }
    else
    {
        file_name = argv[1];
        sgmt = atoi(argv[2]);
        num_of_childer = atoi(argv[3]);
    }

    // open file
    FILE *fp = fopen(file_name, "r");
    int num_lines = 0;
    if (fp == NULL)
    {
        printf("This file does not exist.\n");
        exit(1);
    }

    // count file's lines
    num_lines = countLines(fp);
    // to get pointer at beggining
    fseek(fp, 0, SEEK_SET);

    // create array of lines
    char **array_of_txt_lines;
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

    // i will use an array(representing the different segments) of arrays of text lines
    // if sgmt is 0 then we have only 1 segment with all the lines in it
    // array(*) of arrays(**) of strings(char***)
    segment **array_of_sgmt = malloc(sgmt * sizeof(segment *));

    for (int i = 0; i < sgmt; i++)
    {
        array_of_sgmt[i] = malloc(sizeof(segment));
        array_of_sgmt[i]->array_of_lines = NULL;
        array_of_sgmt[i]->num_of_lines = 0;
    }

    // every segment grabs a line from the array, until there are no left
    int rem_lines = num_lines;
    while (rem_lines > 0)
    {
        for (int i = 0; i < sgmt; i++)
        {
            if (rem_lines == 0)
                break;
            array_of_sgmt[i]->num_of_lines++;
            array_of_sgmt[i]->array_of_lines = realloc(array_of_sgmt[i]->array_of_lines, array_of_sgmt[i]->num_of_lines * sizeof(char **));
            array_of_sgmt[i]->array_of_lines[array_of_sgmt[i]->num_of_lines - 1] = &array_of_txt_lines[num_lines - rem_lines];
            rem_lines--;
        }
    }

    // the idea is that I will create an 1D char array containing the lines of each segmen
    // then I will pass this array through the shared memory
    // let's make a test

    int num_of_lines_per_segment = num_lines / sgmt;
    int shmid, semid;
    sharedMem *shmem;

    if ((shmid = shmget(SHMKEY, num_of_lines_per_segment * MAX_LINE_LENGTH * sizeof(char), PERMS | IPC_CREAT)) < 0)
    {
        perror("shmget");
        exit(1);
    }
    printf("Creating shared memory with ID: %d\n", shmid);

    if ((shmem = shmat(shmid, NULL, 0)) == (void *)-1)
    {
        perror("shmem");
        exit(1);
    }
    printf("Attaching shared memory segment. \n");

    // test
    char segment_to_pass[num_of_lines_per_segment * MAX_LINE_LENGTH];

    for (int i = 0; i < num_of_lines_per_segment * MAX_LINE_LENGTH; i++)
    {
        segment_to_pass[i] = '\0';
    }

    for (int i = 0; i < array_of_sgmt[0]->num_of_lines; i++)
    {
        strcat(segment_to_pass, *(array_of_sgmt[0]->array_of_lines[i]));
    }

    // initializing semaphores
    // beacuse we need multiple processes to gain access of the shared memory,
    // we cant use binary semaphores, thus
    // we need counting semaphores...
    char *sem_name = "sem_";
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

    // passing arguments
    char *shmkey_char;
    char *sgmt_char;
    char *num_of_lines_per_segment_char;

    itoa(sgmt, &sgmt_char);
    itoa(num_of_lines_per_segment, &num_of_lines_per_segment_char);
    itoa((int)SHMKEY, &shmkey_char);

    // mating ;)
    int pid;
    for (int i = 0; i < num_of_childer; i++)
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
            char *args_to_send[] = {"./child", shmkey_char, sgmt_char, num_of_lines_per_segment_char, sem_name, NULL};
            if (execv(args_to_send[0], args_to_send) < 0)
            {
                perror("Execv");
            }
        }
    }
}