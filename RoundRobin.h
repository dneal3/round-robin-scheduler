#ifndef ROUND_ROBIN_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <wait.h>
#include <unistd.h>
#include <signal.h>

// MACROS
#define READY 0
#define RUNNING 1
#define STOPPED 2
#define DONE 3
#define EXITED 4
#define BUFF_SIZE 256
// --

struct TaskController
{
    // Honestly not 100% why this exists it doesnt need to worked it into my solution early in this project, and just kind of rolled with it, could easily be removed and just use a PCB** global.
	int numtasks;
	struct ProcessControlBlock** tasklist;
};

struct ProcessControlBlock
{
    // This struct mimics the Process control block of a process, much simpler though.
	char* cmd;
	char** args;
	pid_t pid;
	int status;
};

void processTasks(char* filename);
void executeTasks(void);
void freePCB(void);
void waitForProcesses(void);
void sigalrm_handler(int sig);
void sigchld_handler(int sig);
void readProcessInfo(void);
#endif
