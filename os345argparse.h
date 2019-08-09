#ifndef OS345ARGPARSE_H_
#define OS345ARGPARSE_H_

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "os345.h"

#define MAX_ARG_LEN		256

char* bufferContents;
int nextCharPos;

typedef struct {
	int argc;
	char** argv;
	bool runInBackground;
	int errors;
} ParsedLine;

ParsedLine parseArgs(char* buffer);
int parseLine(int* newArgc, char** newArgv, bool* background);
int parseArg(int* newArgc, char** newArgv);
int parseQuotedString(int* newArgc, char** newArgv);
int parseQuoteDelimitedString(int* newArgc, char** newArgv);
int parseString(int* newArgc, char** newArgv);
int parseNum(char* str);
int parseWhiteSpace(int* newArgc, char** newArgv);
int parseAmpersand(bool* background);
char peekChar();
char popChar();
bool isStringChar(char c);
bool isQuote(char c);

ParsedLine parseArgs(char* buffer) {
	debugPrint('p', 'f', "parseArgs()\n");
	bufferContents = buffer;
	nextCharPos = 0;


	ParsedLine line;
	line.argc = 0;
	line.argv = malloc(MAX_ARGS * sizeof(char*));
	for (int i = 1; i < MAX_ARGS; i++) line.argv[i] = NULL;
	line.runInBackground = FALSE;

	line.errors = parseLine(&line.argc, line.argv, &line.runInBackground);
	debugPrint('p', 'e', "finished parsing. errors:%d\n", line.errors);


	return line;
}

int parseLine(int* newArgc, char** newArgv, bool* background) {
	debugPrint('p', 'f', "parseLine()\n");
	int errors = 0;
	while (peekChar() && peekChar() != '&') {
		errors += parseWhiteSpace(newArgc, newArgv);
		errors += parseArg(newArgc, newArgv);
	}
	errors += parseAmpersand(background);
	errors += parseWhiteSpace(newArgc, newArgv);
	if (peekChar() != '\0')
		errors++;
	return errors;
}

int parseArg(int* newArgc, char** newArgv) {
	debugPrint('p', 'f', "parseArg()\n");
	if (isQuote(peekChar())) {
		return parseQuotedString(newArgc, newArgv);
	}
	else {
		return parseString(newArgc, newArgv);
	}
}

int parseQuotedString(int* newArgc, char** newArgv) {
	debugPrint('p', 'f', "parseQuotedString()\n");
	if (!isQuote(peekChar())) {
		return 1;
	}
	else {
		char openQuote = popChar();
		int errors = 0;
		errors = parseQuoteDelimitedString(newArgc, newArgv);
		if (!errors && openQuote == peekChar()) {
			popChar();
			return 0;
		}
		else {
			return ++errors;
		}
	}
}

int parseQuoteDelimitedString(int* newArgc, char** newArgv) {
	debugPrint('p', 'f', "parseQuoteDelimitedString()\n");
	char arg[MAX_ARG_LEN] = "";
	char nextChar;
	while (peekChar() != '\0') {
		nextChar = peekChar();
		if (isStringChar(nextChar) || isspace(nextChar)) {
			nextChar = popChar();
			strncat(arg, &nextChar, 1);   //append the new char
		}
		else if (isQuote(nextChar)) {
			newArgv[(*newArgc)] = malloc((strlen(arg) + 1) * sizeof(char));
			strcpy(newArgv[(*newArgc)], arg);
			debugPrint('p', 'm', "malloced Arg %s\n", newArgv[(*newArgc)]);
			(*newArgc)++;
			return 0;
		}
		else {
			return 1;
		}
	}
	return 1;
}

int parseString(int* newArgc, char** newArgv) {
	debugPrint('p', 'f', "parseString()\n");
	char arg[MAX_ARG_LEN] = "";
	char nextChar;
	while (1) {
		nextChar = peekChar();
		if (isStringChar(nextChar)) {
			nextChar = tolower(popChar());
			strncat(arg, &nextChar, 1);   //append the new char
		}
		else if (isspace(nextChar) || nextChar == '\0' || nextChar == '&') {
			newArgv[(*newArgc)] = malloc((strlen(arg) + 1) * sizeof(char));
			strcpy(newArgv[(*newArgc)], arg);
			debugPrint('p', 'm', "malloced Arg %s\n", newArgv[(*newArgc)]);
			(*newArgc)++;
			return 0;
		}
		else {
			return 1;
		}
	}
	return 1;
}

int parseWhiteSpace(int* newArgc, char** newArgv) {
	debugPrint('p', 'f', "parsewhiteSpace()\n");
	while (isspace(peekChar())) {
		popChar();
	}
	return 0;
}

int parseAmpersand(bool* background) {
	debugPrint('p', 'f', "parseAmpersand()\n");
	if (peekChar() != '&') {
		return 0;
	}
	else {
		popChar();
		*background = TRUE;
		return 0;
	}
}


char peekChar() {
	debugPrint('p', 'p', "peeked %c\n", bufferContents[nextCharPos]);
	return bufferContents[nextCharPos];
}

char popChar() {
	debugPrint('p', 'p', "popped %c\n", bufferContents[nextCharPos]);
	if (bufferContents[nextCharPos])
		nextCharPos++;
	return bufferContents[nextCharPos - 1];
}

bool isStringChar(char c) {
	if (isalpha(c)) {
		return 1;
	}
	else if (isdigit(c)) {
		return 1;
	}
	else if (c == '.') {
		return 1;
	}
	else if (c == ':') {
		return 1;
	}
	else if (isspace(c)) {
		return 0;
	}
	else if (c == '\0') {
		return 0;
	}
	else if (c == '&') {
		return 0;
	}
	return 1;
}

bool isQuote(char c) {
	if (c == '\'') {
		return 0;
	}
	else if (c == '"') {
		return 1;
	}
	return 0;
}

/* format:
 *  2x..	binary
 *  8x..	octal
 *  0x..	hex
 *    ..	decimal
 */

int parseNum(char* str) {
	int num;
	char* waste = NULL;
	char c = str[0];
	if (isdigit(c) && str[1] == 'x') {	// look for escape sequence
		if (c == '2') {
			num = strtol(&str[2], &waste, 2);
		}
		else if (c == '8') {
			num = strtol(&str[2], &waste, 8);
		}
		else if (c == '0') {
			num = strtol(&str[2], &waste, 16);
		}
	}
	else {
		num = strtol(str, &waste, 10);
	}
	if ((*waste) != '\0') {
		printf("%s is not a number\n", waste);
		num = 0;
	}
	return num;
}

#endif /* OS345ARGPARSE_H_ */