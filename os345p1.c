// os345p1.c - Command Line Processor 01/20/2017
// ***********************************************************************
// **   DISCLAMER ** DISCLAMER ** DISCLAMER ** DISCLAMER ** DISCLAMER   **
// **                                                                   **
// ** The code given here is the basis for the CS345 projects.          **
// ** It comes "as is" and "unwarranted."  As such, when you use part   **
// ** or all of the code, it becomes "yours" and you are responsible to **
// ** understand any algorithm or method presented.  Likewise, any      **
// ** errors or problems become your responsibility to fix.             **
// **                                                                   **
// ** NOTES:                                                            **
// ** -Comments beginning with "// ??" may require some implementation. **
// ** -Tab stops are set at every 3 spaces.                             **
// ** -The function API's in "OS345.h" should not be altered.           **
// **                                                                   **
// **   DISCLAMER ** DISCLAMER ** DISCLAMER ** DISCLAMER ** DISCLAMER   **
// ***********************************************************************
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include <assert.h>
#include "os345.h"
#include "os345signals.h"
#include "os345argparse.h"
#include "pq.h"

// The 'reset_context' comes from 'main' in os345.c.  Proper shut-down
// procedure is to long jump to the 'reset_context' passing in the
// power down code from 'os345.h' that indicates the desired behaviour.

extern jmp_buf reset_context;
extern TCB tcb[];
extern int curTask;

// ***********************************************************************
// project 1 global variables
//
extern long swapCount;					// number of scheduler cycles
extern char inBuffer[];					// character input buffer
extern Semaphore* inBufferReady;		// input buffer ready semaphore
extern bool diskMounted;				// disk has been mounted
extern char dirPath[];					// directory path
Command** commands;						// shell commands


// ***********************************************************************
// project 1 prototypes
Command** P1_init(void);
Command* newCommand(char*, char*, int (*func)(int, char**), char*);

void mySigIntHandler()
{
	printf("Hellomynameisinigomontoyayoukilledmyfatherpreparetodie");
}

// ***********************************************************************
// myShell - command line interpreter
//
// Project 1 - implement a Shell (CLI) that:
//
// 1. Prompts the user for a command line.
// 2. WAIT's until a user line has been entered.
// 3. Parses the global char array inBuffer.
// 4. Creates new argc, argv variables using malloc.
// 5. Searches a command list for valid OS commands.
// 6. If found, perform a function variable call passing argc/argv variables.
// 7. Supports background execution of non-intrinsic commands.
//
int P1_shellTask(int argc, char* argv[])
{
	int i, found, newArgc, memI;					// # of arguments
	char** newArgv;							// pointers to arguments
	const char amp[2] = "&";

	// initialize shell commands
	commands = P1_init();					// init shell commands

	while (1)
	{
		bool backgroundTask = FALSE;
		// output prompt
		if (diskMounted) printf("\n%s>>", dirPath);
		else printf("\n%ld>>", swapCount);

		semWait(inBufferReady);			// wait for input buffer semaphore
		if (!inBuffer[0]) continue;		// ignore blank lines
		// printf("%s", inBuffer);

		SWAP										// do context switch

		{
			// ?? >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
			// ?? parse command line into argc, argv[] variables
			// ?? must use malloc for argv storage!
			static char* sp, *myArgv[MAX_ARGS];

		// init arguments
		newArgc = 0;
		sp = inBuffer;				// point to input string
		for (i = 1; i < MAX_ARGS; i++)
			myArgv[i] = 0;

		// parse input string

		int start = 0, end = 0, len = (int)strlen(sp);
		while (start < len) {
			while (start < len && isspace(sp[start])) {
				start++;
			}

			while (end < len + 1
				   && (end <= start
						|| (sp[start] == '"' && (isspace(sp[end]) == 0 || sp[end - 1] != '"'))
						|| (sp[start] != '"' && isspace(sp[end]) == 0))
			) {
				end++;
			}

			if (start == end) {
				break;
			}

			myArgv[newArgc] = (char*)malloc(sizeof(char) * (end - start + 1));
			int nA = 0;
			while (start < end) {
				myArgv[newArgc][nA++] = sp[start++];
			}
			myArgv[newArgc][nA] = '\0';
			start++;
			newArgc++;
		}

		newArgv = (char**)malloc(sizeof(char*) * newArgc); // malloc returns a void * to the allocated memory or on failure returns a null pointer
		if (newArgv == NULL) { exit(1); }                    // if null malloc failed exit the program
		for (memI = 0; memI < newArgc; memI++) {
			if (myArgv[memI][0] == 0) {
				newArgc--;
				continue;
			}

			if (memI == (newArgc - 1) && strcmp(myArgv[memI],amp) == 0) {
				// last argument is an &
				backgroundTask = TRUE;
				newArgc--;
				break;
			}

			newArgv[memI] = myArgv[memI];

			// encounter double quote string
			if (newArgv[memI][0] == '"' && newArgv[memI][strlen(newArgv[memI]) - 1] == '"') {
				// if the arg is a string
				// in this case we do nothing to transform the string
				continue;
			}

			//convert the string to lowercase to ease comparisons
			for (int sI = 0; newArgv[memI][sI]; sI++) {
				newArgv[memI][sI] = tolower(newArgv[memI][sI]);
			}
		}
		}	// ?? >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

			SWAP										// do context switch

		// look for command
		for (found = i = 0; i < NUM_COMMANDS; i++)
		{
			if (!strcmp(newArgv[0], commands[i]->command) ||
				!strcmp(newArgv[0], commands[i]->shortcut))
			{
				// command found
				if (backgroundTask) {
					int tid = createTask(
						&(*commands[i]->description),
						&(*commands[i]->func),
						tcb[0].priority,
						newArgc,
						newArgv
					);

					if (tid == -1) {
						printf("\nTCB is full");
					}
				}
				else {
					int retValue = (*commands[i]->func)(newArgc, newArgv);
					if (retValue) printf("\nCommand Error %d", retValue);
				}

				found = TRUE;
				break;
			}
		}
		SWAP										// do context switch
			if (!found)	printf("\nInvalid command!");

		// ?? free up any malloc'd argv parameters
		for (i = 0; i < INBUF_SIZE; i++) inBuffer[i] = 0;

		for (newArgc--; newArgc > -1; newArgc--) {
			free(newArgv[newArgc]);
		}
		SWAP										// do context switch
	}
	return 0;						// terminate task
} // end P1_shellTask


// ***********************************************************************
// ***********************************************************************
// P1 Project
//
#define NUM_ALIVE	3

int P1AliveTask(int argc, char* argv[])
{
	while (1)
	{
		int i;
		printf("\n(%d) ", curTask);
		for (i = 0; i < argc; i++) printf("%s%s", argv[i], (i < argc) ? " " : "");
		for (i = 0; i < 100000; i++) swapTask();
	}
	return 0;						// terminate task
} // end P1AliveTask

int P1_project1(int argc, char* argv[])
{
	int p1_mode = 0;
	// check if just changing P1 mode
	if (argc > 1)
	{
		p1_mode = atoi(argv[1]);
	}
	switch(p1_mode)
	{
		default:
		case 0:
		{
			int j;
			for (j = 0; j < 4; j++)
			{
				int i;
				printf("\nI'm Alive %d,%d", curTask, j);
				for (i = 0; i < 100000; i++) swapTask();
			}
			break;
		}

		case 1:
		{
			int i;
			char buffer[16];
			for (i = 0; i < NUM_ALIVE; i++)
			{
				sprintf(buffer, "I'm Alive %d", i);
				createTask(buffer,			// task name
						P1AliveTask,		// task
						LOW_PRIORITY,		// task priority
						argc,				// task argc
						argv);				// task argument pointers
			}
		}
		break;
	}
	return 0;
} // end P1_project1

int P1_add(int argc, char* argv[])
{
	int total = 0;
	for (int i = 1; i < argc; i++) 
	{
		printf("\noperand: %d", parseNum(argv[i]));
		total += parseNum(argv[i]);
	}		
	
	printf("\n=> SUM = %d\n", total);
	return 0;
} // end P1_add

int P1_args(int argc, char** argv)
{
	printf("\n=> Argc = %d\nparsed arguments:", argc);
	for (int i = 0; i < argc; i++) {
		printf("\n\t|%s|", argv[i]);
	}
	return 0;
} // end args

// ***********************************************************************
// ***********************************************************************
// quit command
//
int P1_quit(int argc, char* argv[])
{
	int i;

	// free P1 commands
	for (i = 0; i < NUM_COMMANDS; i++)
	{
		free(commands[i]->command);
		free(commands[i]->shortcut);
		free(commands[i]->description);
	}
	free(commands);

	// powerdown OS345
	longjmp(reset_context, POWER_DOWN_QUIT);
	return 0;
} // end P1_quit



// **************************************************************************
// **************************************************************************
// lc3 command
//
int P1_lc3(int argc, char* argv[])
{
	printf("P1_lc3");
	strcpy (argv[0], "0");
	return lc3Task(argc, argv);
} // end P1_lc3



// ***********************************************************************
// ***********************************************************************
// help command
//
int P1_help(int argc, char* argv[])
{
	int i;

	// list commands
	for (i = 0; i < NUM_COMMANDS; i++)
	{
		SWAP										// do context switch
		if (strstr(commands[i]->description, ":")) printf("\n");
		printf("\n%4s: %s", commands[i]->shortcut, commands[i]->description);
	}

	return 0;
} // end P1_help


// ***********************************************************************
// ***********************************************************************
// initialize shell commands
//
Command* newCommand(char* command, char* shortcut, int (*func)(int, char**), char* description)
{
	Command* cmd = (Command*)malloc(sizeof(Command));

	// get long command
	cmd->command = (char*)malloc(strlen(command) + 1);
	strcpy(cmd->command, command);

	// get shortcut command
	cmd->shortcut = (char*)malloc(strlen(shortcut) + 1);
	strcpy(cmd->shortcut, shortcut);

	// get function pointer
	cmd->func = func;

	// get description
	cmd->description = (char*)malloc(strlen(description) + 1);
	strcpy(cmd->description, description);

	return cmd;
} // end newCommand


Command** P1_init()
{
	int i  = 0;
	Command** commands = (Command**)malloc(sizeof(Command*) * NUM_COMMANDS);

	// system
	commands[i++] = newCommand("quit", "q", P1_quit, "Quit");
	commands[i++] = newCommand("kill", "kt", P2_killTask, "Kill task");
	commands[i++] = newCommand("reset", "rs", P2_reset, "Reset system");

	// P1: Shell
	commands[i++] = newCommand("project1", "p1", P1_project1, "P1: Shell");
	commands[i++] = newCommand("help", "he", P1_help, "OS345 Help");
	commands[i++] = newCommand("lc3", "lc3", P1_lc3, "Execute LC3 program");
	commands[i++] = newCommand("args", "a", P1_args, "Print given Args");
	commands[i++] = newCommand("add", "ad", P1_add, "Add all Args");

	// P2: Tasking
	commands[i++] = newCommand("project2", "p2", P2_project2, "P2: Tasking");
	commands[i++] = newCommand("semaphores", "sem", P2_listSems, "List semaphores");
	commands[i++] = newCommand("tasks", "lt", P2_listTasks, "List tasks");
	commands[i++] = newCommand("signal1", "s1", P2_signal1, "Signal sem1 semaphore");
	commands[i++] = newCommand("signal2", "s2", P2_signal2, "Signal sem2 semaphore");

	// P3: Jurassic Park
	commands[i++] = newCommand("project3", "p3", P3_project3, "P3: Jurassic Park");
	commands[i++] = newCommand("deltaclock", "dc", P3_dc, "List deltaclock entries");

	// P4: Virtual Memory
	commands[i++] = newCommand("project4", "p4", P4_project4, "P4: Virtual Memory");
	commands[i++] = newCommand("frametable", "dft", P4_dumpFrameTable, "Dump bit frame table");
	commands[i++] = newCommand("initmemory", "im", P4_initMemory, "Initialize virtual memory");
	commands[i++] = newCommand("touch", "vma", P4_vmaccess, "Access LC-3 memory location");
	commands[i++] = newCommand("stats", "vms", P4_virtualMemStats, "Output virtual memory stats");
	commands[i++] = newCommand("crawler", "cra", P4_crawler, "Execute crawler.hex");
	commands[i++] = newCommand("memtest", "mem", P4_memtest, "Execute memtest.hex");

	commands[i++] = newCommand("frame", "dfm", P4_dumpFrame, "Dump LC-3 memory frame");
	commands[i++] = newCommand("memory", "dm", P4_dumpLC3Mem, "Dump LC-3 memory");
	commands[i++] = newCommand("page", "dp", P4_dumpPageMemory, "Dump swap page");
	commands[i++] = newCommand("virtual", "dvm", P4_dumpVirtualMem, "Dump virtual memory page");
	commands[i++] = newCommand("root", "rpt", P4_rootPageTable, "Display root page table");
	commands[i++] = newCommand("user", "upt", P4_userPageTable, "Display user page table");

	// P5: Scheduling
	commands[i++] = newCommand("project5", "p5", P5_project5, "P5: Scheduling");

	// P6: FAT
	commands[i++] = newCommand("project6", "p6", P6_project6, "P6: FAT");
	commands[i++] = newCommand("change", "cd", P6_cd, "Change directory");
	commands[i++] = newCommand("copy", "cf", P6_copy, "Copy file");
	commands[i++] = newCommand("define", "df", P6_define, "Define file");
	commands[i++] = newCommand("delete", "dl", P6_del, "Delete file");
	commands[i++] = newCommand("directory", "dir", P6_dir, "List current directory");
	commands[i++] = newCommand("mount", "md", P6_mount, "Mount disk");
	commands[i++] = newCommand("mkdir", "mk", P6_mkdir, "Create directory");
	commands[i++] = newCommand("run", "run", P6_run, "Execute LC-3 program");
	commands[i++] = newCommand("space", "sp", P6_space, "Space on disk");
	commands[i++] = newCommand("type", "ty", P6_type, "Type file");
	commands[i++] = newCommand("unmount", "um", P6_unmount, "Unmount disk");

	commands[i++] = newCommand("fat", "ft", P6_dfat, "Display fat table");
	commands[i++] = newCommand("fileslots", "fs", P6_fileSlots, "Display current open slots");
	commands[i++] = newCommand("sector", "ds", P6_dumpSector, "Display disk sector");
	commands[i++] = newCommand("chkdsk", "ck", P6_chkdsk, "Check disk");
	commands[i++] = newCommand("final", "ft", P6_finalTest, "Execute file test");

	commands[i++] = newCommand("open", "op", P6_open, "Open file test");
	commands[i++] = newCommand("read", "rd", P6_read, "Read file test");
	commands[i++] = newCommand("write", "wr", P6_write, "Write file test");
	commands[i++] = newCommand("seek", "sk", P6_seek, "Seek file test");
	commands[i++] = newCommand("close", "cl", P6_close, "Close file test");

	assert(i == NUM_COMMANDS);

	return commands;

} // end P1_init
