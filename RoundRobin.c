// Made by Derrick Neal, Completed 4 November 2018
#include "RoundRobin.h"

// GLOBALS all marked with g_ as first two chars.
struct TaskController* g_Control; // struct defined in .h file
int g_Run = 0;
int g_Numtasks = 0;
int g_Taskscompleted = 0;
int g_Taskid = 0;
// --

void processTasks(char* workloadlist)
{
	/*
		This function will take a FILE* that points to the task list that it needs to parse and it will return a list of jobs each in an array that contains it arguments.
		Something along the lines of work[0] = ["./anagram", "A.dict", "A.out"]
	*/
	struct TaskController* control = (struct TaskController*)malloc(sizeof(struct TaskController));	
	FILE*  work_in = NULL;
	char*  curr_cmd = (char*)calloc(BUFF_SIZE, sizeof(char));
	char** fakeargv = (char**)calloc(BUFF_SIZE, sizeof(char*));
	struct ProcessControlBlock* task = (struct ProcessControlBlock*)malloc(sizeof(struct ProcessControlBlock)*BUFF_SIZE);
	int    str_index = 0;
	int    cmd_index = 0;
	int    task_index = 0;
	char   c = NULL;
	control->tasklist = (struct ProcessControlBlock**)malloc(sizeof(struct ProcessControlBlock)*BUFF_SIZE);
    
    // Assign FILE ptr while checking to see if it is NULL
	if((work_in = fopen(workloadlist, "r")) == NULL)
	{
		fprintf(stderr,"ERROR: FILE DOES NOT EXIST\n");
		exit(0);
	}
    
    // assign char while checking for EOF
	while((c = fgetc(work_in)) != EOF)
	{
		switch(c)
		{
			case '\n' :
				curr_cmd[str_index] = '\0';
				fakeargv[cmd_index] = curr_cmd;
				fakeargv[cmd_index+1] = NULL;
				task->cmd = strdup(fakeargv[0]);
				task->args = fakeargv;
				control->tasklist[task_index] = task;
				task_index++;
				task = (struct ProcessControlBlock*)malloc(sizeof(struct ProcessControlBlock)*BUFF_SIZE);
				fakeargv = (char**)calloc(BUFF_SIZE, sizeof(char*));
				curr_cmd = (char*)calloc(BUFF_SIZE,sizeof(char));
				cmd_index = 0;
				str_index = 0;
				break;

			case ' ' :
				curr_cmd[str_index] = '\0';
				fakeargv[cmd_index] = curr_cmd;
				curr_cmd = (char*)calloc(BUFF_SIZE, sizeof(char));
				cmd_index++;
				str_index = 0;
				break;

			default :
				curr_cmd[str_index] = c;
				str_index++;
				break;
		}
	}
	 
	control->tasklist[task_index] = NULL;
	g_Numtasks = task_index-1;
	g_Control = control;
	
	fclose(work_in);
	free(task);
	free(curr_cmd);
	free(fakeargv);
}

void freePCB()
{
    //Free the TaskController and it's prcess control blocks from memory alloc'd during the creation process
	int i = 0;
	int j = 0;
	if(g_Control != NULL)
	{
		while(g_Control->tasklist[i] != NULL)
		{
			free(g_Control->tasklist[i]->cmd);
			while(g_Control->tasklist[i]->args[j] != '\0')
			{
				free(g_Control->tasklist[i]->args[j]);
				j++;
			}
			free(g_Control->tasklist[i]->args);
			free(g_Control->tasklist[i]);
			i++;
			j=0;
		}
		i=0;
		free(g_Control->tasklist);
		free(g_Control);
	}
}


void waitForProcesses()
{
	//simple way to wait for all tasks to be done without being dependent on certain pids
	while(g_Taskscompleted != g_Numtasks);
}

void readProcessInfo()
{
    // Reports a little about the process being run once it is stopped and the PCB can be read uses the global taskid to check what was just stopped and report back the name, CPU time, memory alloc'd and time spent waiting for I/O.
		int c = 0;
		char buff[BUFF_SIZE]; 
		FILE * stats;
    //If the process is done or exited the stat file doesnt exist so we'd get null, may as well check to make sure it isnt real, even tho technically through my implementation this is useless. Honestly this logic isnt even right I think.
		if((g_Control->tasklist[g_Taskid]->status != DONE) || (g_Control->tasklist[g_Taskid]->status != EXITED))
		{
            
			sprintf(buff, "/proc/%d/stat", g_Control->tasklist[g_Taskid]->pid);
            // Assign file ptr an check if null
			if((stats = fopen(buff, "r")) != NULL)
			{
                // Use fscanf with the conevention of string then a space as thats how we want to read in words from the file, since I know what lines I was looking for I could have just moved the file pointer in the file using fseek as opposed to looping through it, but dont care that much so it's fine for now.
				while((fscanf(stats, "%s[SPACE]", buff) != EOF))
				{
					switch(c) //After reading th man page of /proc/pid/stat these are the only lines that really matter in giving a general view of information
					{
						case 1:
							fprintf(stdout, "Name of process: %s\n", buff);
							c++;
							break;
					
						case 14:
							fprintf(stdout, "CPU Time: %s\n", buff);
							c++;
							break;
						
						case 24:
							fprintf(stdout, "Memory Used: %s\n", buff);
							c++;
							break;
						
						case 42:
							fprintf(stdout, "Time waiting for IO: %s\n\n", buff);
							c++;
							break;
						
						default:
							c++;
							break;
					}
					if(c > 42)
					{
						break;
					}	
				}
			}
			fclose(stats);
		}

}


void sigalrm_handler(int sig)
{
    // Crutial for the round robin part of this project, once the alarm signal is received this will rotate processes by stopping the current process via SIGSTOP signal, and will continue the next unfinished task using a SIGCONT signal.
	if(g_Control->tasklist[g_Taskid]->status == RUNNING)
	{
		g_Control->tasklist[g_Taskid]->status = STOPPED;
		kill(g_Control->tasklist[g_Taskid]->pid, SIGSTOP);
		readProcessInfo();
		g_Taskid++;
	}
	if(g_Taskid >= g_Numtasks)
	{
		g_Taskid = 0;
	}
	for(; g_Taskid<g_Numtasks; g_Taskid++)
	{

		if((g_Control->tasklist[g_Taskid]->status == STOPPED) || (g_Control->tasklist[g_Taskid]->status == READY))
		{
			g_Control->tasklist[g_Taskid]->status = RUNNING;
			kill(g_Control->tasklist[g_Taskid]->pid, SIGCONT);
			break;
		}

	}
	alarm(1);
}

void sigchld_handler(int sig)
{
    // This function handles the SIGCHLD signal by checkint to see of the signal has finished or not, if it has it marks the status of the child as done, which will then be used by the alarm handler in order to select the next unfinished task.
	int status;
	if(waitpid(g_Control->tasklist[g_Taskid]->pid, &status, WNOHANG) > 0)
	{
		g_Control->tasklist[g_Taskid]->status = DONE;
		g_Taskscompleted++;
	}
}

void executeTasks()
{
    // forks the parent and makes a new process to run each task. Stores the PID of each task and makes its status ready, handles if execvp fails or if fork fails.
		pid_t pid = -1;
		int i = 0;
		while(g_Control->tasklist[i] != NULL)
		{
			pid = fork();
			g_Control->tasklist[i]->status = READY;
			if(pid < 0)
			{
				freePCB();
				exit(0);
			}

			if ((pid == 0))
			{
				raise(SIGSTOP);
				execvp(g_Control->tasklist[i]->cmd, g_Control->tasklist[i]->args);
				g_Control->tasklist[i]->status = EXITED;
				freePCB();
				exit(0);
			}
			g_Control->tasklist[i]->pid = pid;
			i++;

		}

}
int main(int argc, char**argv)
{
	signal(SIGALRM, &sigalrm_handler);
	signal(SIGCHLD, &sigchld_handler);
	processTasks(argv[1]); 
	executeTasks();
	alarm(1);
	waitForProcesses();
	freePCB();
	fprintf(stdout, "All processes finished\n");
	return 0;
}
