#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

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

int main(int argc, char *argv[])
{
    char *file_name = argv[1];
    int sgmt = atoi(argv[2]);

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

    // first decide how many arrays you will want, based on the number of sgmt
    if (sgmt > num_lines)
    {
        printf("Segmentation is greater than the number of lines.\n");
        exit(1);
    }

    if (sgmt == 0)
    {
        sgmt = 1;
    }

    // i will use an array(representing the different segments) of arrays of text lines
    // if sgmt is 0 then we have only 1 segment with all the lines in it
    // array(*) of arrays(**) of strings(char***)
    char ***sgmt_of_txt = (char **)malloc(sgmt * sizeof(char **));
    int num_lines_per_sgmt = num_lines / sgmt;
    int mod = num_lines % sgmt;
    int i;
    for (i = 0; i < sgmt - 1; i++)
    {
        sgmt_of_txt[i] = (char *)malloc(num_lines_per_sgmt * sizeof(char *));
    }

    // the last segment will contain the remaining lines (mod) if exist + the lines the corresponding lines
    sgmt_of_txt[i] = (char *)malloc((num_lines_per_sgmt + mod) * sizeof(char *)); // now i will have the value of (sgmt-1)
    
}