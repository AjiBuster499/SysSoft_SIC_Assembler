#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <regex.h>

#define PASS1 0
#define END -2
#define ERROR -1
#define SAFE 0

struct symbols {
	int	DefinedOnSourceLine;
	int	Address;
	char	Name[7];
};
typedef struct symbols	SYMBOL;

struct instructions {
	char name[10];
	int hexCode;
};
typedef struct instructions OPCODE;

int IsADirective (char *Test);
int IsAnInstruction (char *Test);
int IsAValidSymbol (char *TestSymbol, char *prevSymbol);
