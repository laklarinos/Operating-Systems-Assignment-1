#ifndef HELPERS
#define HELPERS
#define SHMSIZE 500
#define MAX_LINE_LENGTH 1024
#define MAX_SGMT 20
#define MAX_LINES_PER_SEGMENT 100
#define SHMKEY (key_t)4321
#define PERMS IPC_CREAT | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <semaphore.h>

typedef struct sharedMem
{
    char buffer[MAX_LINES_PER_SEGMENT][MAX_LINE_LENGTH];
    int requested_sgmt;
    int array_of_read_count[MAX_SGMT];
} sharedMem;

typedef struct segment
{
    // array of pointers to string/lines
    char ***array_of_lines;
    int num_of_lines;
} segment;
int countDigits(int number);
void *create_array_of_txt_lines(FILE *fp, char ***array, int number_of_lines);
int countLines(FILE *fp);
void itoa(int i, char **ptr);
void readFromSharedMem(char **buffer, sharedMem *sharedStuff, sem_t *emptySemaphore, sem_t *fullSemaphore);
void writeToSharedMem(char *message, sharedMem *sharedStuff, sem_t *emptySemaphore, sem_t *fullSemaphore);
#endif