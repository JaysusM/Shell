/**
UNIX Shell Project

Sistemas Operativos
Grados I. Informatica, Computadores & Software
Dept. Arquitectura de Computadores - UMA

Some code adapted from "Fundamentos de Sistemas Operativos", Silberschatz et al.

To compile and run the program:
   $ gcc Shell_project.c job_control.c -o Shell
   $ ./Shell
	(then type ^D to exit program)

**/

#include "job_control.h"   // remember to compile with module job_control.c
#include <string.h>

#define MAX_LINE 256 /* 256 chars per line, per command, should be enough. */

job* joblist;

void bg(char *args) {
	job *selectedJob;
	int pos;

	if(args == NULL)
		if(empty_list(joblist) == 1) {
			printf("Process not found");
			return;
		} else
			pos = list_size(joblist);
	else if(atoi(args) > list_size(joblist)) {
		printf("Process not found: %d", atoi(args));
		return;
	} else
		pos = atoi(args);

		block_SIGCHLD();
		selectedJob = get_item_bypos(joblist, pos);
		unblock_SIGCHLD();
		if(selectedJob->state == STOPPED)
			kill(-(selectedJob->pgid), SIGCONT);
		selectedJob->state = BACKGROUND;
}

void fg(char *args) {
	int pos;
	job* selectedJob;
	int status;
	int info;
	job *auxJob;

	if(args == NULL)
		if(empty_list(joblist)) {
			printf("Process not found");
			return;
		} else
			pos = list_size(joblist);
	else if(atoi(args) > list_size(joblist)) {
		printf("Process not found: %d", atoi(args));
		return;
	} else
		pos = atoi(args);

	block_SIGCHLD();
	auxJob = get_item_bypos(joblist, pos);
	selectedJob = new_job(auxJob->pgid, auxJob->command, auxJob->state);
	delete_job(joblist, auxJob);
	unblock_SIGCHLD();
	set_terminal(selectedJob->pgid);
	if(selectedJob->state == STOPPED)
		kill(-(selectedJob->pgid), SIGCONT);
	selectedJob->state = FOREGROUND;
	waitpid(selectedJob->pgid, &status, WUNTRACED);
	set_terminal(getpid());
	enum status status_res = analyze_status(status,	&info);
	printf("Foreground pid: %i, command %s, %s, info: %i",
	selectedJob->pgid, selectedJob->command, status_strings[status_res], info);
  if(status_res == SUSPENDED){
    selectedJob->state = STOPPED;
		block_SIGCHLD();
		add_job(joblist, selectedJob);
		unblock_SIGCHLD();
	}
}

void SIGCHLD_handler(int signal) {
	block_SIGCHLD();
	int status, info;

	for(int i = 1; i <= list_size(joblist); i++) {
		job* currentjob = get_item_bypos(joblist, i);
		int pid_wait = waitpid(currentjob->pgid, &status, WNOHANG | WUNTRACED | WCONTINUED);

		if(pid_wait) {
			enum status state = analyze_status(status, &info);
			if(state == EXITED || info == SIGKILL || info == SIGTERM) {
					delete_job(joblist, currentjob);
					i--;
			} else if (state == CONTINUED)
				currentjob->state = BACKGROUND;
		}

	}
	unblock_SIGCHLD();
}

// -----------------------------------------------------------------------
//                            MAIN
// -----------------------------------------------------------------------

int main(void)
{
	char inputBuffer[MAX_LINE]; /* buffer to hold the command entered */
	int background;             /* equals 1 if a command is followed by '&' */
	char *args[MAX_LINE/2];     /* command line (of 256) has max of 128 arguments */
	// probably useful variables:
	int pid_fork, pid_wait; /* pid for created and waited process */
	int status;             /* status returned by wait */
	enum status status_res; /* status processed by analyze_status() */
	int info;				/* info processed by analyze_status() */

	joblist = new_list("job list");

	signal(SIGCHLD, SIGCHLD_handler);

	new_process_group(getpid());
	ignore_terminal_signals();


	while (1)   /* Program terminates normally inside get_command() after ^D is typed*/
	{
		printf("\nShell$ ");
		fflush(stdout);
		get_command(inputBuffer, MAX_LINE, args, &background);  /* get next command */

		if(args[0]==NULL) continue;   // if empty command

		/* the steps are:
			 (1) fork a child process using fork()
			 (2) the child process will invoke execvp()
			 (3) if background == 0, the paren0t will wait, otherwise continue
			 (4) Shell shows a status message for processed command
			 (5) loop returns to get_commnad() function
		*/

	if(strcmp(args[0], "bg") == 0)
	{
		bg(args[1]);
	}
	else if(strcmp(args[0], "fg") == 0)
	{
		fg(args[1]);
	}
	else if(strcmp(args[0], "jobs") == 0)
	{
			block_SIGCHLD();
			print_job_list(joblist);
			unblock_SIGCHLD();
	}	else if(strcmp(args[0], "cd") == 0)
	{
		char* path;
		//If cd command doesn't give us a path, we set a default one.
		args[1] == NULL ? path = "/" : args[1];
		//chdir will return -1 if an error occurs
			if(chdir(path) == -1)
				printf("Path does not exist");
	} else {
		pid_fork = fork();
		if(pid_fork < 0) {
			printf("Error creating child process");
			continue;
		} else if(pid_fork) {
			if(!background) {
				set_terminal(pid_fork);
				pid_wait = waitpid(pid_fork, &status, WUNTRACED);
				status = analyze_status(status, &info);
				set_terminal(getpid());
				if(info != 255) {
					if(status_strings[status] == "Suspended")
						add_job(joblist, new_job(pid_fork, args[0], STOPPED));
				//	else
				//		add_job(joblist, new_job(pid_fork, args[0], BACKGROUND);

					printf("Foreground pid: %i, command %s, %s, info: %i",
					pid_fork, args[0], status_strings[status], info);
				}
			} else {
				add_job(joblist, new_job(pid_fork, args[0], BACKGROUND));
				printf("Background job running... pid: %d, command: %s", pid_fork, args[0]);
			}
		} else {
			new_process_group(getpid());
			restore_terminal_signals();
			if(execvp(args[0], args)) {
				printf("Command not valid...");
				exit(-1);
			}
			exit(0);
		}
	}

	} // end while
}
