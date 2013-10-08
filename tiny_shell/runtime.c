/***************************************************************************
 *  Title: Runtime environment 
 * -------------------------------------------------------------------------
 *    Purpose: Runs commands
 *    Author: Stefan Birrer
 *    Version: $Revision: 1.1 $
 *    Last Modification: $Date: 2005/10/13 05:24:59 $
 *    File: $RCSfile: runtime.c,v $
 *    Copyright: (C) 2002 by Stefan Birrer
 ***************************************************************************/
/***************************************************************************
 *  ChangeLog:
 * -------------------------------------------------------------------------
 *    $Log: runtime.c,v $
 *    Revision 1.1  2005/10/13 05:24:59  sbirrer
 *    - added the skeleton files
 *
 *    Revision 1.6  2002/10/24 21:32:47  sempi
 *    final release
 *
 *    Revision 1.5  2002/10/23 21:54:27  sempi
 *    beta release
 *
 *    Revision 1.4  2002/10/21 04:49:35  sempi
 *    minor correction
 *
 *    Revision 1.3  2002/10/21 04:47:05  sempi
 *    Milestone 2 beta
 *
 *    Revision 1.2  2002/10/15 20:37:26  sempi
 *    Comments updated
 *
 *    Revision 1.1  2002/10/15 20:20:56  sempi
 *    Milestone 1
 *
 ***************************************************************************/
	#define __RUNTIME_IMPL__

  /************System include***********************************************/
	#include <assert.h>
	#include <errno.h>
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
	#include <sys/wait.h>
	#include <sys/stat.h>
	#include <sys/types.h>
	#include <unistd.h>
	#include <fcntl.h>
	#include <signal.h>

  /************Private include**********************************************/
	#include "runtime.h"
	#include "io.h"

  /************Defines and Typedefs*****************************************/
    /*  #defines and typedefs should have their names in all caps.
     *  Global variables begin with g. Global constants with k. Local
     *  variables should be in all lower case. When initializing
     *  structures and arrays, line everything up in neat columns.
     */

  /************Global Variables*********************************************/

	#define NBUILTINCOMMANDS (sizeof BuiltInCommands / sizeof(char*))

	typedef struct bgjob_l {
		pid_t pid;
		struct bgjob_l* next;
	} bgjobL;

	/* the pids of the background processes */
	bgjobL *bgjobs = NULL;
    static fg_job = 0;
    /************Function Prototypes******************************************/
	/* run command */
	static void RunCmdFork(commandT*, bool);
	/* runs an external program command after some checks */
	static void RunExternalCmd(commandT*, bool);
	/* resolves the path and checks for exutable flag */
	static bool ResolveExternalCmd(commandT*);
	/* forks and runs a external program */
	static void Exec(commandT*, bool);
	/* runs a builtin command */
	static void RunBuiltInCmd(commandT*);
	/* checks whether a command is a builtin command */
	static bool IsBuiltIn(char*);
    /*Util function to debug;*/
    static void PrintBGJobs();
  /************External Declaration*****************************************/

/**************Implementation***********************************************/
    int total_task;
	void RunCmd(commandT** cmd, int n)
	{
      int i;
      total_task = n;
      if(n == 1)
          RunCmdFork(cmd[0], TRUE);
      else{
        RunCmdPipe(cmd, n);
        for(i = 0; i < n; i++)
            ReleaseCmdT(&cmd[i]);
      }
	}

	void RunCmdFork(commandT* cmd, bool fork)
	{
		if (cmd->argc<=0)
			return;
		if (IsBuiltIn(cmd->argv[0]))
		{
			RunBuiltInCmd(cmd);
		}
		else
		{
			RunExternalCmd(cmd, fork);
		}
	}

	void RunCmdBg(commandT* cmd)
	{
		// TODO
        //PrintBGJobs();
	}

	void RunCmdPipe(commandT** cmd, int n )
{
    //Validate the cma1 and cmd2.
    int READ=0, WRITE=1;
    int i = 0;
    int A_B[2];
    
    for(i=0;i<n;i++){
        if(!ResolveExternalCmd(cmd[i])){
            fprintf(stderr,"%s command not found\n", cmd[i]->argv[0]);
            return;   
        }    
    }

    pipe(A_B);

    pid_t pid = fork();
    if ( pid == 0 ) {//1st child proc: A
        dup2(A_B[WRITE], 1); //stdout
        execv(cmd[0]->name, cmd[0]->argv);
    } 
    waitpid(-1,NULL,WNOHANG);
    /*else {
        wait(NULL);
    } */

    for(i=1;i<n;i++){
        //fds for 2 process;
        pid = fork();
        assert(pid >=0);
        if (pid == 0 ) {//2nd child proc: B
            dup2(A_B[READ], 0); //stdin
            if(i!=n-1)
                dup2(A_B[WRITE], 1); // stdout
            execv(cmd[i]->name, cmd[i]->argv);
        } else {
            waitpid(-1,NULL,WNOHANG);
        }
    }
}

	void RunCmdRedirOut(commandT* cmd, char* file)
	{
	}

	void RunCmdRedirIn(commandT* cmd, char* file)
	{
	}


/*Try to run an external command*/
static void RunExternalCmd(commandT* cmd, bool fork)
{
  if (ResolveExternalCmd(cmd)){ 
    //if (cmd -> bg == 1) RunCmdBg(cmd);
    Exec(cmd, fork);
  }
  else {
    printf("%s: command not found\n", cmd->argv[0]);
    fflush(stdout);
    ReleaseCmdT(&cmd);
  }
}

/*Find the executable based on search list provided by environment variable PATH*/
static bool ResolveExternalCmd(commandT* cmd)
{
  char *pathlist, *c;
  char buf[1024];
  int i, j;
  struct stat fs;

  if(strchr(cmd->argv[0],'/') != NULL){
    if(stat(cmd->argv[0], &fs) >= 0){
      if(S_ISDIR(fs.st_mode) == 0)
        if(access(cmd->argv[0],X_OK) == 0){/*Whether it's an executable or the user has required permisson to run it*/
          cmd->name = strdup(cmd->argv[0]);
          return TRUE;
      }
    }
    return FALSE;
  }
  pathlist = getenv("PATH");
  if(pathlist == NULL) return FALSE;
  i = 0;
  while(i<strlen(pathlist)){
    c = strchr(&(pathlist[i]),':');
    if(c != NULL){
      for(j = 0; c != &(pathlist[i]); i++, j++)
	buf[j] = pathlist[i];
      i++;
    }
    else{
      for(j = 0; i < strlen(pathlist); i++, j++)
	buf[j] = pathlist[i];
    }
    buf[j] = '\0';
    strcat(buf, "/");
    strcat(buf,cmd->argv[0]);
    if(stat(buf, &fs) >= 0){
      if(S_ISDIR(fs.st_mode) == 0)
	if(access(buf,X_OK) == 0){/*Whether it's an executable or the user has required permisson to run it*/
	  cmd->name = strdup(buf); 
	  return TRUE;
	}
    }
  }
  return FALSE; /*The command is not found or the user don't have enough priority to run.*/
}

	static void Exec(commandT* cmd, bool forceFork)
	{
        if(forceFork){
            pid_t pid = fork();
            if(pid<0){
                fprintf(stderr,"fork() failed at Exec().\n");
                return;
            }
            if(pid ==0){ // child process.
                execv(cmd->name,cmd->argv);
                return;
            }else { // parent process.
                if (cmd -> bg == 1) { //A command with &, so parent will keep executing.
                    //Append the child process into list;
                    bgjobL *bgjob = (bgjobL*) malloc (sizeof(bgjobL));
                    bgjob -> pid = pid;
                    bgjob -> next = NULL;
                    
                    int bgJobNumber = 1;

                    if (bgjobs == NULL) bgjobs = bgjob;
                    else {
                        bgjobL *current = bgjobs;
                        while (1) {
                            if (current -> next == NULL) {
                                current -> next = bgjob;
                                bgJobNumber++;
                                break;
                            }
                            current = current -> next;
                            bgJobNumber++;
                        } 
                    }
                    printf("[%d] %d \n", bgJobNumber, pid); 
                   // wait(NULL); 
                    //int id = waitpid(pid, NULL, WNOHANG); 
                    //printf("\n================== %d", id );
                    // if (id == pid) printf("ri o!");
                   // if(id == 0 ) printf("i m dead.");
                } else { // Parent process wait for child terminate.
                    fg_job = pid;
                    wait(NULL); 
                }
            }

        }
	}

        static bool IsBuiltIn(char* cmd)
        {
            //Check four built-in commands -- bg, fg, jobs, alias.
            if (strcmp(cmd, "bg") == 0) return TRUE;
            if (strcmp(cmd, "fg") == 0) return TRUE;
            if (strcmp(cmd, "jobs") == 0) return TRUE;
            if (strcmp(cmd, "alias") == 0) return TRUE;
        	return FALSE;     
        }

   
	static void RunBuiltInCmd(commandT* cmd)
	{
            //NEED modified;
            if (strcmp(cmd -> cmdline, "bg") == 0) {
                printf("bg need implement.");    
            }
	}

        void CheckJobs()
	{
           // PrintBGJobs();
 	}


commandT* CreateCmdT(int n)
{
  int i;
  commandT * cd = malloc(sizeof(commandT) + sizeof(char *) * (n + 1));
  cd -> name = NULL;
  cd -> cmdline = NULL;
  cd -> is_redirect_in = cd -> is_redirect_out = 0;
  cd -> redirect_in = cd -> redirect_out = NULL;
  cd -> argc = n;
  for(i = 0; i <=n; i++)
    cd -> argv[i] = NULL;
  return cd;
}

/*Release and collect the space of a commandT struct*/
void ReleaseCmdT(commandT **cmd){
  int i;
  if((*cmd)->name != NULL) free((*cmd)->name);
  if((*cmd)->cmdline != NULL) free((*cmd)->cmdline);
  if((*cmd)->redirect_in != NULL) free((*cmd)->redirect_in);
  if((*cmd)->redirect_out != NULL) free((*cmd)->redirect_out);
  for(i = 0; i < (*cmd)->argc; i++)
    if((*cmd)->argv[i] != NULL) free((*cmd)->argv[i]);
  free(*cmd);
}

/*********************Utils****************************************/
void PrintBGJobs() {
    if (bgjobs == NULL) printf("No background job!");
    else {
        bgjobL *current = bgjobs;
        int count = 0;
        while (1) {
        if (current -> next == NULL) {
                printf ("Job id: %d \n", count);
                break;
        }
        printf ("Job id:  %d \n", count );
        current = current -> next;
        count++;
        } 
    }
}
