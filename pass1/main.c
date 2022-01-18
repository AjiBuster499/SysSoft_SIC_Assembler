#include "headers.h"

int end(FILE*); // generic end function
int isSymbolLine(char*, int*); // check if line has symbol
void addToStruct (SYMBOL*, int, char*, int, int); // add the symbol to the structure
void printLine(SYMBOL*, char*, int, int, int*); // print out the symbols

int main(int argc, char* argv[]) {
	if (argc != 2) { // Didn't pass a file
		printf("ERROR: Usage: %s filename\n", argv[0]);
		return 0;
	}
	// Create all your variables.
	FILE *asm_file = fopen(argv[1], "r");
	SYMBOL *symbols = malloc (1024 * sizeof(SYMBOL));
	char buf[4096]; // oversized buffer
	char *symbol, *directive, *operand; // the three stooges of SIC
	char *prevSymbols;
	char delims[2] = "\t";
	int counter = 0, reti, lineNum = 0; // address set to 0, returned int for regex, and line number
	int symbolsIndex = 0; // Indexer for Struct
	regex_t regex;

	// prep the regex
	reti = regcomp(&regex, "^[a-fA-F0-9]+$", REG_EXTENDED);
	if (reti) { // regex failed to compile
		printf("Regex Compilation failed.\n");
	}

	if (asm_file == NULL) { // File non-existent
		printf("ERROR: File not found or could not be read %s\n", argv[0]);
		return end(asm_file);
	}

	// prepping the char pointers
	{
		prevSymbols = malloc(2048 * sizeof(char));
		memset(prevSymbols, '\0', 2048 * sizeof(char));
	}

	while (!feof(asm_file)) { // Main Parsing Loop
		lineNum++; // Increment the line number
		fgets(buf, 4096, asm_file); // grab the line
		if (counter >= 32768) { // goes outside SIC Memory range
			printf("ERROR: OUT OF MEMORY\n at line %d.\n", lineNum);
			return end(asm_file);
		}

		if (isSymbolLine(buf, &counter) == 0) { // not a Symbol Line
			continue;
		}

		symbol = strtok(buf, delims); // grab the symbol

		if (IsAValidSymbol(symbol, prevSymbols) == 0) { // Not a valid symbol
			printf("ERROR: INVALID SYMBOL:\n at %s on line %d\n", buf, lineNum);
			return end(asm_file);
		}

		directive = strtok(NULL, delims); // grab the directive
		operand = strtok(NULL, delims); // grab the operand

/*
		* Begin Parsing the Line for Real
*/
		if (strcmp(directive, "START") == 0) { // the directive is START, aka the first directive
			counter += strtol(operand, NULL, 16); // set the initial counter to START's address
			printLine(symbols, symbol, counter, lineNum, &symbolsIndex);
			continue; // don't need to keep going
		}
		printLine(symbols, symbol, counter, lineNum, &symbolsIndex);

		if (strcmp(directive, "END") == 0) { // this is the final line
			return end(asm_file);
		} else if (strcmp(directive, "RESB") == 0) { // Reserving a variable amount of bytes
			counter += strtol(operand, NULL, 10); // add that to the counter.
		} else if (strcmp(directive, "RESW") == 0) { // word is 3 bytes
			counter += 3* strtol(operand, NULL, 10);
		} else if (strcmp(directive, "BYTE") == 0) {
/*
			* Clearing out the junk in the operand
*/
			char op_value[32];
			memset(op_value, '\0', 32 * sizeof(char));
			strncpy(op_value, operand + 2, strlen(operand) - 2);
			char* last = strchr(op_value, '\'');
			last[0] = '\0';
			if (operand[0] == 'C') {
				// need to add a byte for every character
				counter += strlen(op_value);
			} else if (operand[0] == 'X') {
				// need to add a byte for every 2 chars.
				reti = regexec(&regex, op_value, 0, NULL, 0); // execute the regex
				if (reti != 0) { // there was an invalid match
					printf("ERROR: %s is not a valid hex value\n  at line %d\n", op_value, lineNum);
					return end(asm_file);
				}
				counter += ceil(strlen(op_value)) / 2; // increment the counter
			}
		} else if (strcmp(directive, "WORD") == 0) {
			if ((strtol(operand, NULL, 10)) > pow(2, 23)) { // the 24th bit is used for sign
				printf("ERROR: WORD TOO LARGE\n at line %d\n", lineNum);
				return end(asm_file);
			}
			counter += 3;
		} else { // just a regular line
			counter += 3;
		}
	}
	return end(asm_file); // does this even ever get executed??
}

int end (FILE* fp) {
	fclose(fp);
	return 0;
}

int isSymbolLine(char* buf, int* counter) {  // check if line has symbol
	// Check if the line has a symbol
	int result = 0; // default: not symbol line
	if (buf[0] == '\t') { // no symbol, but is an assembly line
		*counter += 3;
	} else if (buf[0] == '#') { // is a comment
		// do nothing
	} else { // is a symbol assembly line
		result = 1;
	}

	return result;
}

void addToStruct (SYMBOL *symbols, int line, char *symbol, int counter, int symbolsIndex) {
	symbols[symbolsIndex].DefinedOnSourceLine = line;
	symbols[symbolsIndex].Address = counter;
	strcpy(symbols[symbolsIndex].Name, symbol);
}

void printLine(SYMBOL* symbols, char* symbol, int counter, int line, int* symbolsIndex) {
	printf("%s\t%X\n", symbol, counter);
	addToStruct(symbols, line, symbol, counter, *symbolsIndex);
	(*symbolsIndex)++;
}
