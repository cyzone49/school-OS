#include <stdio.h>
#include <stdarg.h>
#include "os345.h"

bool printParser = 0;		// -p
bool printParserReads = 0;	// -r
bool printInterrupts = 0;	// -i
bool printInterruptKeystrokes = 0;// -k
bool printSignals = 0;		// -s

bool printfnNames = 0;		// -f
bool printMallocs = 0;		// -m
bool printErrors = 0;		// -e

void debugPrint(char category, char type, char* fmt, ...) {
//	printf("\n\ndebugPrint called...\n");

	va_list args;
	va_start(args, fmt);
	if (category == 'p' && printParser) {
		if (type == 'p' && printParserReads) {
			vprintf(fmt, args);		//print parser peeks and pops
		}
		else {
			vprintf(fmt, args);		//print parser details
		}
	}
	else if (category == 'i' && printInterrupts) {
		if (type == 'k' && printInterruptKeystrokes) {
			vprintf(fmt, args);		//print keystrokes
		}
		else {
			vprintf(fmt, args);		//print interrupts details
		}
	}
	else if (category == 's' && printSignals) {
		vprintf(fmt, args);		//print Signals
	}
	else if (type == 'f' && printfnNames) {
		vprintf(fmt, args);		//print function names
	}
	else if (type == 'm' && printMallocs) {
		vprintf(fmt, args);		//print mallocs
	}
	else if (type == 'e' && printErrors) {
		vprintf(fmt, args);		//print errors
	}
	va_end(args);
}