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

void itoa(int i, char** ptr)
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
    return lines;
}

void *create_array_of_txt_lines(FILE *fp, char ***array, int number_of_lines)
{
    *array = malloc(number_of_lines * sizeof(char *));
    for (int i = 0; i < number_of_lines; i++)
    {
        (*array)[i] = malloc((MAX_LINE_LENGTH + 1) * sizeof(char));
    }

    int j = 0;
    while (j < number_of_lines)
    {
        if (fgets((*array)[j], MAX_LINE_LENGTH, fp) == NULL)
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