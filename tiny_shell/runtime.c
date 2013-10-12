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
#include <pwd.h>
#include <unistd.h>
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
/*
   typedef struct alias_l {
   char* key;
   char* value;
   struct alias_l* next;
   } aliasL;  
 */
/* the pids of the background processes */
bgjobL *bgjobs = NULL;
//  aliasL *aliasList = NULL;
int fg_job = 0;
char * fgCmd = NULL;
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
/************External Declaration*****************************************/
/*Util function to debug;*/
// static void PrintBGJobs();
/*pop and push a job from and into bgjob list*/
static bgjobL* PopBGJob();
//static int PushBGJob();
/*Alias and Unalias functions*/
//  static void PrintAlias();
//  static void AddAlias(commandT*);
//  static bool isAlias(commandT* cmd);
// static commandT* ParseAliasCmd(commandT* cmd);
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
    int k = 0;
    for(k=0;k<n;k++){
        if(!ResolveExternalCmd(cmd[k])){
            printf("%s command not found\n", cmd[k]->argv[0]);
            return;   
        } 
    }
    
    int A_B[2];
    pipe(A_B);
    int pid;
    pid = fork();

    if (pid == 0) {
        close(A_B[0]);
        dup2(A_B[1], 1);
        execv(cmd[0] -> name, cmd[0] -> argv);
    } close(A_B[1]);

    //wait(NULL);
    pid = fork(); 

    if (pid == 0) {
        dup2(A_B[0], 0);
        execv(cmd[1] -> name, cmd[1] -> argv);
    }

    wait(NULL);
    wait(NULL);
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
static void waitfg(int id){
    while(fg_job) 
        sleep (1);
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
            setpgid(0, 0);
            int defout=-1;
            int fd= -1;
            if(cmd->is_redirect_out){
                defout = dup(1);
                fd=open(cmd->redirect_out, O_RDWR|O_CREAT,
                        S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                if(fd == -1){
                    fprintf(stderr,"create file %s failed\n.",cmd->redirect_out);
                    return;    
                }
                dup2(fd, 1);
            }

            int defin = -1;
            int fd_in = -1;
            if(cmd->is_redirect_in){
                defin = dup(0);
                fd_in = open(cmd->redirect_in, O_RDONLY,S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

                if(fd_in == -1){
                    fprintf(stderr,"create file %s failed\n.",cmd->redirect_in);
                    return;    
                }
                dup2(fd_in,0);
            }

            execv(cmd->name,cmd->argv);
            if(cmd->is_redirect_out){
                dup2(defout, 1); //restore  
                close(fd);
                close(defout);
            }
            if(cmd->is_redirect_in){
                dup2(defin,0); // restore
                close(fd_in);
                close(defin);
            }
            fflush(stdout);
            exit(0);
        }else { // parent process.
            if (cmd -> bg == 1) { //A command with &, so parent will keep executing.
                //Append the child process into list;
                
                //printf("childPID:%d\n",pid);

                bgjobL *bgjob = (bgjobL*) malloc (sizeof(bgjobL));
                bgjob -> pid = pid;
                bgjob -> state = RUNNING;
                int size = strlen(cmd->cmdline);
                char * pa = malloc(size+1);
                strcpy(pa, cmd->cmdline);
                bgjob -> cmd = pa;
                bgjob -> next = NULL;
                AddToBgJobs(bgjob);
            } else { // Parent process wait for child terminate.
                fg_job = pid;
                if(fgCmd!=NULL)
                    free(fgCmd);
                int size = strlen (cmd->cmdline);
                fgCmd = malloc(size+1);
                strcpy(fgCmd, cmd->cmdline); 
                waitfg(pid);
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
    if (strcmp(cmd, "cd") == 0) return TRUE;
    return FALSE;     
}


static void RunBuiltInCmd(commandT* cmd)
{
    if (strcmp(cmd -> argv[0], "bg") == 0) {
        if(bgjobs == NULL || bgjobs->next==NULL)
            return;
        int job_num = 0;
        if (cmd -> argc == 1) {// no arg, find most recent one.
            bgjobL *p = bgjobs->next;
            while(p->next!=NULL){
                p = p->next;    
            }
			job_num = p->job_id;
        } else { 
            job_num = atoi(cmd -> argv[1]);
        }

        bgjobL *p = bgjobs;
		while(p->next !=NULL){
			if(p->job_id == job_num) {
				p->state = STOPPED;
				break;
			}
			p = p->next;
		}
		if(p->job_id!=job_num){
			printf("job not found[bg]\n");
			fflush(stdout);
			return;
		}
        kill(p->pid, SIGCONT);
		p->state = RUNNING;
        return;
    }

    if (strcmp(cmd -> argv[0], "fg") == 0) {
        int job_num = 0;

        if(bgjobs == NULL || bgjobs->next==NULL) // No background job.
            return;

        if (cmd -> argc == 1){ // no arg, find most recent one.
            bgjobL *p = bgjobs->next;
            while(p->next!=NULL){
                p = p->next;    
            }
			job_num = p->job_id;
        } else job_num = atoi(cmd->argv[1]);
       
		bgjobL *proc;
		if (( proc = PopBGJob( job_num)) == 0) 
            return;
        fg_job = proc->pid;
		fgCmd = proc->cmd;
		free(proc);
        
		// may be loop???
		//waitpid(pid, NULL, 0 );
        kill(-fg_job, SIGCONT);
		waitfg(fg_job);
		return;
    }

    if (strcmp(cmd -> cmdline, "jobs") == 0) {
        if(bgjobs == NULL || bgjobs->next==NULL)
            return;
        else {
            bgjobL * p= bgjobs->next;
            while(1){
                if(p->state == RUNNING){
                    if(p->cmd[strlen(p->cmd)-1]==' ')
						printf("[%d]   %s                 %s&\n",p->job_id, "Running",p->cmd);
					else
						printf("[%d]   %s                 %s &\n",p->job_id, "Running",p->cmd);

				}
                else if(p->state == STOPPED)
                    printf("[%d]   %s                 %s\n",p->job_id, "Stopped",p->cmd);
                else if(p->state ==DONE)
					printf("[%d]   %s                    %s\n",p->job_id, "Done",p->cmd);
                fflush(stdout);
                if(p->next==NULL)
                    break;
                p=p->next;
            }            
        }
        return;
    }

    /***************************************
     *
     *For alias and unalias commands;
     *
     * ************************************/
    /*if (strcmp(cmd -> argv[0], "alias") == 0) {
      printf("cmdline: %s \n", cmd->cmdline);
      printf("argv1: %s \n", cmd->argv[0]);
      printf("argv2: %s \n", cmd->argv[1]);
      printf("argv3: %s \n", cmd->argv[2]);    

      if (cmd -> argc == 1) //List all alias;
      PrintAlias();
      else 
      AddAlias(cmd);
      }*/

     // if (strcmp(cmd -> argv[0], "unalias") == 0) {}          
	if (strcmp(cmd -> argv[0], "cd") == 0) {
		int rc;
		if(cmd->argc == 1) { // go to home
			struct passwd *pw = getpwuid(getuid());

			const char *homedir = pw->pw_dir;
			rc = chdir(homedir);
		} else {
			rc = chdir(cmd->argv[1]);    
		} 
		if(rc < 0){
			fprintf(stderr,"cd error\n");			
		}
		return;      
	}          
}

bgjobL* PopBGJob(int n) { //Pop a job from bgjobs list and return its pid;
    bgjobL *p= bgjobs->next, *pre = bgjobs;
    while(p->next!=NULL){
		if(p->job_id == n)
			break;
        p= p->next;
        pre= pre->next;
    }
	if(p->job_id != n)
		return 0;

    pre->next = p->next;	
    return p;
}

/*int PushBGJob(pid_t pid) { // Push a into bgjob list and return its jobs number;
  return 0;
  }*/

int AddToBgJobs(bgjobL *p){
    if(bgjobs==NULL){
        bgjobs = malloc(sizeof(bgjobL));
        bgjobs->next = p;
        p->job_id = 1;
		return 1;
    }
    int num = 0;
    bgjobL *pt = bgjobs;
    while(pt->next !=NULL){
        pt=pt->next;
        num++;
    }
    pt->next = p;
	p->job_id = pt->job_id + 1;
    return p->job_id;
}

void CheckJobs()
{
    //int job_num = 1;
    if (bgjobs == NULL || bgjobs -> next == NULL) 
        return;
    bgjobL *parent = bgjobs;
    bgjobL *current = parent -> next;

    while (current != NULL) {
        if(current->state == DONE){
            printf("[%d]   %s                    %s\n",current->job_id, "Done",current->cmd);
            fflush(stdout);
            parent->next = current->next;
            free(current->cmd);
            free(current);
            //current = parent->next;
        }
        current = current -> next;
        parent = parent -> next;
        //job_num++;
    }       
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
/*
   void PrintBGJobs() {
   if (bgjobs == NULL) return;
   bgjobL* head = bgjobs;
   if (head->next == NULL) {
   printf("No background job!");
   return;
   }

   bgjobL *current = head->next;
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
   }*/
/*
   void PrintAlias() {
   return;
   }

   void AddAlias(commandT* cmd) {

//Generate original command text and alias command text.
char *originCmd = malloc(sizeof(char) * 100);
char *aliasCmd = malloc(sizeof(char) * 100);
int oCmd_size = 0;
int aCmd_size = 0;
int i =6; //hardcode to ignore "alias" from comline.
bool isLeftSide = TRUE;
for (i; i<=strlen(cmd->cmdline); i++) {
char current = cmd->cmdline[i];

if (isLeftSide && current != '=') {
originCmd[oCmd_size] = current;
oCmd_size++;
} 
else if (isLeftSide && current =='=') {
originCmd[oCmd_size] = 0;
isLeftSide = FALSE;
}
else if (!isLeftSide && current != '\'') {
aliasCmd[aCmd_size] = current;
aCmd_size++;
}
else if (!isLeftSide && current == '\'') {
aliasCmd[aCmd_size] = 0;
}
}
printf("o: %s \n", originCmd);
printf("a: %s \n", aliasCmd);

//A new alias.
aliasL *alias = malloc(sizeof(aliasL));
alias -> key = originCmd;
alias -> value = aliasCmd;
alias -> next = NULL;

//Insert the alias in the aliasL.
if (aliasList == NULL) {
aliasList = (aliasL *)malloc(sizeof(aliasL));
}

}

bool isAlias(commandT* cmd) {

}

commandT* ParseAliasCmd(commandT* cmd) {

commandT* new_cmd = malloc(sizeof(commandT)+ sizeof(char *)* 100);
int i = 0;
int new_arg_count = 0;
bool inputQuoted = FALSE;
for(i=0; i<cmd->argc; i++)
{
char* cur_arg = cmd->argv[i];
if(strchr(cur_arg, ' '))
inputQuoted = TRUE;
char* new = is_alias_for(cur_arg);
char* tmp = malloc(sizeof(char) * MAXLINE);
int tmp_len = 0;
int j;
for(j=0; j<=strlen(new); j++)
{
char cur_char = new[j];
if(((cur_char == ' ' || cur_char == 0) && !inputQuoted) || (inputQuoted && cur_char == 0))
{
    // a new argument is ended
    tmp[tmp_len] = 0;
    tmp_len = 0;
    char* new_arg = malloc(sizeof(char) *(tmp_len + 1));
    //  printf("new arg: %s\n", new_arg); garbage line?
    strcpy(new_arg, tmp);
    new_cmd->argv[new_arg_count] = new_arg;
    // printf("new argv: %s\n", new_cmd->argv[new_arg_count]);
    new_arg_count++;
    tmp = realloc(tmp, sizeof(char) * MAXLINE);
}         
else
{
    tmp[tmp_len] = cur_char;
    tmp_len++;
}
}
inputQuoted = FALSE;
free(tmp);
//free(cur_arg); freeCommand will handle this after this func returns
}
new_cmd->name = new_cmd->argv[0];
new_cmd->argc = new_arg_count;
new_cmd->argv[new_cmd->argc] = 0;
freeCommand(cmd);
return new_cmd;    
}
*/
