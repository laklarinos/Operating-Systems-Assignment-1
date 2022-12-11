#ifndef INCLUDES
    #define INCLUDES
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <unistd.h>
    #include <ctype.h>
    #include <sys/shm.h>
    #include <semaphore.h>
    #include <sys/stat.h>
    #include <sys/types.h>
    #include <fcntl.h>
    #include <sys/wait.h>
    #include <time.h> 
    #include <limits.h>
    #include <errno.h>
    #define SEM_PERMS (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)

    typedef struct sharedMem
    {
        char* buffer;
    } sharedMem;

    typedef struct segment
    {
        // array of pointers to string-lines
        char ***array_of_lines;
        int num_of_lines;
    } segment;
#endif