/***************************************************************************
 *  Title: MySimpleShell 
 * -------------------------------------------------------------------------
 *    Purpose: A simple shell implementation 
 *    Author: Stefan Birrer
 *    Version: $Revision: 1.1 $
 *    Last Modification: $Date: 2005/10/13 05:24:59 $
 *    File: $RCSfile: tsh.c,v $
 *    Copyright: (C) 2002 by Stefan Birrer
 ***************************************************************************/
/***************************************************************************
 *  ChangeLog:
 * -------------------------------------------------------------------------
 *    $Log: tsh.c,v $
 *    Revision 1.1  2005/10/13 05:24:59  sbirrer
 *    - added the skeleton files
 *
 *    Revision 1.4  2002/10/24 21:32:47  sempi
 *    final release
 *
 *    Revision 1.3  2002/10/23 21:54:27  sempi
 *    beta release
 *
 *    Revision 1.2  2002/10/15 20:37:26  sempi
 *    Comments updated
 *
 *    Revision 1.1  2002/10/15 20:20:56  sempi
 *    Milestone 1
 *
 ***************************************************************************/
#define __MYSS_IMPL__

/************System include***********************************************/
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>    
#include <stdio.h>
/************Private include**********************************************/
#include "tsh.h"
#include "io.h"
#include "interpreter.h"
#include "runtime.h"

/************Defines and Typedefs*****************************************/
/*  #defines and typedefs should have their names in all caps.
 *  Global variables begin with g. Global constants with k. Local
 *  variables should be in all lower case. When initializing
 *  structures and arrays, line everything up in neat columns.
 */

#define BUFSIZE 80

/************Global Variables*********************************************/

/************Function Prototypes******************************************/
/* handles SIGINT and SIGSTOP signals */	
static void sig(int);
static void sigchld_handler(int);
extern int fg_job;
extern char * fgCmd;    
extern bgjobL *bgjobs;
/************External Declaration*****************************************/

/**************Implementation***********************************************/

int main (int argc, char *argv[])
{
    /* Initialize command buffer */
    char* cmdLine = malloc(sizeof(char*)*BUFSIZE);

    /* shell initialization */
    if (signal(SIGINT, sig) == SIG_ERR) PrintPError("SIGINT");
    if (signal(SIGTSTP, sig) == SIG_ERR) PrintPError("SIGTSTP");
    if (signal(SIGCHLD, sigchld_handler) == SIG_ERR) PrintPError("SIGCHLD");

    while (!forceExit) /* repeat forever */
    {
        /* read command line */
        getCommandLine(&cmdLine, BUFSIZE);

        if(strcmp(cmdLine, "exit") == 0)
        {
            forceExit=TRUE;
            continue;
        }

        /* checks the status of background jobs */
        CheckJobs();

        /* interpret command and line
         * includes executing of commands */
        Interpret(cmdLine);

    }
    if(bgjobs != NULL){
        bgjobL * p = bgjobs;
        bgjobL * tmp;
        while(p!=NULL){
            tmp = p->next;
            //if(p->state == RUNNING)
            //	kill(-p->pid,SIGINT);
            free(p->cmd);
            free(p);
            p = tmp;    
        }    
    }
    /* shell termination */
    free(cmdLine);
    fflush(stdout);
    //sleep(1);
    return 0;
} /* end main */


static void sigchld_handler(int signo){
    pid_t pid;
    int status;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        //printf(">>>>>>>>>PID EXIT %d, fg: %d\n",pid, fg_job);
        //fflush(stdout);
        if(WIFEXITED(status)){
            if (pid == fg_job){ 
                fg_job = 0;
                return;
            } 
            if(bgjobs == NULL || bgjobs->next==NULL)
                return;
            else {
                bgjobL * pre = bgjobs;		
                bgjobL * p= bgjobs->next;
                while(p!=NULL){
                    if(p->pid == pid){
                        p->state = DONE;
                        return;
                    }
                    p=p->next;
                    pre=pre->next;
                }            
            }

        }
    } 
    return;
}


static void sig(int signo)
{
    if(signo == SIGINT)
    { 
        if(fg_job){
            kill(-fg_job,SIGINT);
            fg_job = 0;
        }
        return ;
    }

    if( signo ==SIGTSTP ) {
        if(fg_job){
            kill(-fg_job,SIGTSTP);
            bgjobL *p = (bgjobL *)malloc(sizeof(bgjobL));
            p->cmd = fgCmd;
            p->pid = fg_job;
            p->state = STOPPED;
            AddToBgJobs(p);
            fg_job = 0;
            fgCmd = NULL;
        }
        return;
    }           
}

