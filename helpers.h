#ifndef HELPERS
#define HELPERS
#define SHMSIZE 500
#define MAX_LINE_LENGTH 1024
#define SHMKEY (key_t)4321
#define PERMS 0600
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int countDigits(int number);
void *create_array_of_txt_lines(FILE *fp, char ***array, int number_of_lines);
int countLines(FILE *fp);
void itoa(int i, char** ptr);
#endif