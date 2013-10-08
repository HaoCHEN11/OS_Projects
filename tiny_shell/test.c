/* Includes */
#include <unistd.h>     /* Symbolic Constants */
#include <sys/types.h>  /* Primitive System Data Types */ 
#include <errno.h>      /* Errors */
#include <stdio.h>      /* Input/Output */
#include <sys/wait.h>   /* Wait for Process Termination */
#include <stdlib.h>     /* General Utilities */

int main()
{
    pid_t childpid; /* variable to store the child's pid */
    int retval;     /* child process: user-provided return code */
    int status;     /* parent process: child's exit status */

    /* only 1 int variable is needed because each process would have its
       own instance of the variable
       here, 2 int variables are used for clarity */

    /* now create new process */
    childpid = fork();

    if (childpid >= 0) /* fork succeeded */
    {
        if (childpid == 0) /* fork() returns 0 to the child process */
        {
            //exit(retval); /* child exits with user-provided return code */
            execlp("/bin/ls","ls", NULL);
            printf("helloworld\n");
        }
        else /* fork() returns new pid to the parent process */
        {
            wait(NULL);
        }
        printf("test\n");
        fflush(stdout);
    }
    else /* fork returns -1 on failure */
    {
        perror("fork"); /* display error message */
        exit(0); 
    }
}

