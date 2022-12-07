#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <string.h>

#define max_line_length 1024
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
    return lines;
}

void *create_array_of_txt_lines(FILE *fp, char ***array, int number_of_lines)
{
    *array = malloc(number_of_lines * sizeof(char *));
    for (int i = 0; i < number_of_lines; i++)
    {
        (*array)[i] = malloc((max_line_length + 1) * sizeof(char));
    }

    int j = 0;
    while (j < number_of_lines)
    {
        if (fgets((*array)[j], max_line_length, fp) == NULL)
            return NULL;
        (*array)[j] = realloc((*array)[j], ((strlen((*array)[j])) + 1) * sizeof(char));
        if ((*array)[j] == NULL)
        {
            fprintf(stderr, "ERROR\n");
            return NULL;
        }
        j++;
    }
}

typedef struct segment
{
    // array of pointers to string-lines
    char ***array_of_lines;
    int num_of_lines;
} segment;

int main(int argc, char *argv[])
{
    char *file_name;
    // number of segments
    int sgmt;
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
    
}