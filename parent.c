#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <string.h>

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

void create_array_of_txt_lines(FILE *fp, char ***array, int number_of_lines)
{
    *array = (char **)malloc(number_of_lines * sizeof(char *));

    // array to keep track of the number of characters
    // initialize them all with 0s to increase by 1 in each character that is not '\n'
    int array_of_num_of_chars[number_of_lines];
    for (int i = 0; i < number_of_lines; i++)
    {
        array_of_num_of_chars[i] = 0;
    }

    // now count all the char's in each line until eof
    // if c == '\n' dont count it, increase array counter
    // UNTIL END OF FILE (hopefully)
    int counter = 0;
    char c;
    c = fgetc(fp);
    while (c != EOF)
    {
        if (c != '\n')
        {
            array_of_num_of_chars[counter]++;
        }
        else
        {
            counter++;
        }
        c = fgetc(fp);
    }

    for (int i = 0; i < number_of_lines; i++)
    {
        (*array)[i] = (char *)malloc((array_of_num_of_chars[i] + 1) * sizeof(char *));
    }

}

typedef struct segment
{
    char **array_of_lines;
    int num_of_lines;
} segment;

int main(int argc, char *argv[])
{
    char *file_name;
    int sgmt;
    if (argc < 3)
    {
        printf("Please specify a file name and a segmentation number.\n");
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
    create_array_of_txt_lines(fp, &array_of_txt_lines, num_lines);

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
    segment *array_of_sgmt = (segment *)malloc(sgmt * sizeof(segment *));
    int num_lines_per_sgmt = num_lines / sgmt;
    int mod = num_lines % sgmt;

    for (int i = 0; i < sgmt - 1; i++)
    {
        array_of_sgmt[i].array_of_lines = (char **)malloc(num_lines_per_sgmt * sizeof(char *));
        array_of_sgmt[i].num_of_lines = num_lines_per_sgmt;
    }

    // the last segment will contain the remaining lines (mod) if exist + the lines the corresponding lines
    // now i will have the value of (sgmt-1)
    array_of_sgmt[sgmt - 1].array_of_lines = (char **)malloc((num_lines_per_sgmt + mod) * sizeof(char *));
    array_of_sgmt[sgmt - 1].num_of_lines = num_lines_per_sgmt + mod;

    // assign lines to segments
    // for (int i = 0; i < sgmt; i++)
    // {
    //     segment *cur_sgmt = &array_of_sgmt[i];
    //     for (int j = 0; j < cur_sgmt->num_of_lines; j++)
    //     {
    //         int len_of_curr_line =
    //     }
    // }
}