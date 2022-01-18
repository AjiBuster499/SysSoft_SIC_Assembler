#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <regex.h>

struct symbols {
	int	DefinedOnSourceLine;
	int	Address;
	char	Name[7];
};
typedef struct symbols	SYMBOL;

int IsADirective (char *Test);
int IsAnInstruction (char *Test);
int IsAValidSymbol (char *TestSymbol, char *prevSymbol);
