#include "helpers.h"
int countDigits(int n)
{
    int r = 1;
    if (n < 0)
        n = (n == INT_MIN) ? INT_MAX : -n;
    while (n > 9)
    {
        n /= 10;
        r++;
    }
    return r;
}

void itoa(int i, char **ptr)
{
    int length = countDigits(i);
    *ptr = (char *)malloc(sizeof(char) * (length + 1));
    sprintf(*ptr, "%d", i);
}

int countLines(FILE *fp)
{
    int lines = 0;
    int ch = 0;
    while (!feof(fp))
    {
        ch = fgetc(fp);
        if (ch == '\n')
        {
            lines++;
        }
    }
    lines++;
    return lines;
}

void *create_array_of_txt_lines(FILE *fp, char ***array, int number_of_lines)
{
    *array = malloc(number_of_lines * sizeof(char *));
    for (int i = 0; i < number_of_lines; i++)
    {
        (*array)[i] = malloc((MAX_LINE_LENGTH) * sizeof(char));
    }

    int j = 0;
    while (j < number_of_lines)
    {
        if (fgets((*array)[j], MAX_LINE_LENGTH, fp) == NULL)
            return NULL;
        (*array)[j][MAX_LINE_LENGTH - 1] = '\0';
        //(*array)[j] = realloc((*array)[j], ((strlen((*array)[j])) + 1) * sizeof(char));
        //(*array)[j][strlen((*array)[j])] = '\0';
        if ((*array)[j] == NULL)
        {
            fprintf(stderr, "ERROR\n");
            return NULL;
        }
        j++;
    }
}

// void writeToSharedMem(char *message, sharedMem *sharedStuff, sem_t *emptySemaphore, sem_t *fullSemaphore)
// {
//     if (sem_wait(emptySemaphore) < 0)
//     {
//         perror("sem_wait");
//     }

//     // send the num of bytes to allocate

//     // reader should wait for our signal
//     int length = strlen(message) + 1;
//     int lengthOfLength = snprintf(NULL, 0, "%d", length) + 1;
//     char *lengthChar = (char *)malloc(lengthOfLength * sizeof(char));
//     snprintf(lengthChar, lengthOfLength, "%d", length);
//     strcpy(sharedStuff->buffer, lengthChar);

//     if (sem_post(fullSemaphore) < 0)
//     {
//         perror("sem wait");
//     }

//     // we now wait for reader to send a signal
//     if (sem_wait(emptySemaphore) < 0)
//     {
//         perror("sem_wait");
//     }

//     free(lengthChar);
//     // send the message
//     strcpy(sharedStuff->buffer, message);
//     // let the writer know

//     if (sem_post(fullSemaphore) < 0)
//     {
//         perror("sem wait");
//     }

//     if (sem_post(emptySemaphore) < 0)
//     {
//         perror("sem wait");
//     }
//     return;
// }

// void readFromSharedMem(char **buffer, sharedMem *sharedStuff, sem_t *emptySemaphore, sem_t *fullSemaphore)
// {
//     if (sem_wait(fullSemaphore) < 0)
//     {
//         perror("sem_wait");
//     }

//     // send the number of bytes to allocate
//     int bytesToAllocate = atoi(sharedStuff->buffer);
//     *buffer = (char *)malloc((bytesToAllocate + 1) * sizeof(char));
//     sharedStuff->buffer[0] = '\0';
//     //send signal to writer to continue the writing process
//     if (sem_post(emptySemaphore) < 0)
//     {
//         perror("sem wait");
//     }

//     // and wait for the message...
//     if (sem_wait(fullSemaphore) < 0)
//     {
//         perror("sem_wait");
//     }

//     // send the message
//     strcpy(*buffer, sharedStuff->buffer);
//     sharedStuff->buffer[0] = '\0';
//     //printf("READ\n");
//     return;
// }
