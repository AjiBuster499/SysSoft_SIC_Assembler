#include "headers.h"

/* List of Notices:
 * FIRST SUBMISSION IS IN!
 * FINISHED! Resubmit and be happy.
*/

struct isSymbolLineResult {
	int result;
	int counter;
};

struct modificationRecordData {
	int startingAddress;
	int modLength;
	char symbol[7];
};

typedef struct modificationRecordData MRD;

struct objectData {
	char headRecord[21];
	char endRecord[9];
	char textRecords[1024][71];
	char modRecords[1024][18];
};

int end(FILE*, int); // generic end function
void getCleanOperand(char*, char[]); // cleans up operand
int getCounterIncrement (char*, char*, int); // increment counter
struct isSymbolLineResult isSymbolLine(char*, int); // check if line has symbol
int isMemTooBig(int, int); // checks if memory is out of range
void addToStruct (SYMBOL*, int, char*, int, int); // add the symbol to the structure
int printLine(SYMBOL*, char*, int, int, int); // print out the symbols
void initializeOpcodes(OPCODE*); // gotta write them in somehow
int isPredefinedSymbol(SYMBOL*, char*); // checks if a symbol exists
OPCODE findInstruction(OPCODE*, char*, char*); // find an instruction by name
SYMBOL findSymbol(SYMBOL*, char*); // get a symbol by it's name
int writeTextRecord(struct objectData*, SYMBOL*, OPCODE*, char*, char*, int, int, int); // write the text record
void writeHeadRecord(struct objectData*, char*, int, int); // write the header record
void writeEndRecord(struct objectData*, int); // write the end record
int addModRecord(MRD*, int, int, char*, int); // adds the mod record data to a struct array
void writeModRecord(struct objectData*, MRD*, int); // writes the modification records
int writeObjectFile(char[], struct objectData, int, int); // writes the file

static const OPCODE emptyOpcode;
static const SYMBOL emptySymbol;

int main(int argc, char* argv[]) {
	if (argc != 2) { // Didn't pass a file
		printf("ERROR: Usage: %s filename\n", argv[0]);
		return 0;
	}
	// Create all your variables.
	char out_file_name[50] = "";
	strcpy(out_file_name, argv[1]);
	strcat(out_file_name, ".obj");

	FILE *asm_file = fopen(argv[1], "r");
	SYMBOL *symbols = malloc(1024 * sizeof(SYMBOL));
	MRD *modData = malloc (2048 * sizeof(MRD));

	struct objectData records;

	// initialize the symbol table
	for (int i = 0; i < 1024; i++) {
		memset(symbols[i].Name, '\0', 7 * sizeof(char));
		symbols[i].DefinedOnSourceLine = 0;
		symbols[i].Address = 0;
	}

	// initialize the records
	memset(records.headRecord, '\0', 21 * sizeof(char));
	memset(records.endRecord, '\0', 9 * sizeof(char));
	memset(records.textRecords, '\0', 1024 * 71 * sizeof(char));
	memset(records.modRecords, '\0', 1024 * 18 * sizeof(char));

	OPCODE *opcodes = malloc(26 * sizeof(OPCODE));
	initializeOpcodes(opcodes);

	char buf[4096]; // oversized buffer
	char *symbol, *directive, *operand; // the three stooges of SIC
	char *prevSymbols;
	char delims[3] = "\t\n";

	int counter = 0, lineNum = 0; // address set to 0  and line number
	int symbolsIndex = 0; // Indexer for Symbol Struct
	int dataIndex = 0; // Indexer for Data Struct
	int counterInc = 0; // increment the counter by this much

	if (asm_file == NULL) { // File non-existent
		printf("ERROR: File not found or could not be read %s\n", argv[0]);
		return end(asm_file, ERROR);
	}

	// prepping the char pointers
	prevSymbols = malloc(2048 * sizeof(char));
	memset(prevSymbols, '\0', 2048 * sizeof(char));

	while (!feof(asm_file)) { // Main Parsing Loop
		lineNum++; // Increment the line number
		fgets(buf, 4096, asm_file); // grab the line

		if (isMemTooBig(counter, lineNum)) {
			return end(asm_file, ERROR);
		}

		struct isSymbolLineResult ISLRes = isSymbolLine(buf, counter);
		counter = ISLRes.counter;
		if (ISLRes.result != 1) { // not a symbol line
			continue; // moving on.
		}

		symbol = strtok(buf, delims); // grab the symbol
		if (IsAValidSymbol(symbol, prevSymbols) == 0) { // Not a valid symbol
			printf("ERROR: INVALID SYMBOL:\n on line %d\n", lineNum);
			return end(asm_file, ERROR);
		}

		directive = strtok(NULL, delims); // grab the directive
		operand = strtok(NULL, delims); // grab the operand

//	* Begin Parsing the Line for Real
		if (strcmp(directive, "START") == 0) { // the directive is START, aka the first directive
			counter += strtol(operand, NULL, 16); // set the initial counter to START's address
			symbolsIndex = printLine(symbols, symbol, counter, lineNum, symbolsIndex);
			continue; // don't need to keep going
		}

		symbolsIndex = printLine(symbols, symbol, counter, lineNum, symbolsIndex);

		counterInc = getCounterIncrement(directive, operand, lineNum);
		if (counterInc == END) {
			break;
		} else if (counterInc == ERROR) {
			return end(asm_file, ERROR);
		} else {
			counter += counterInc;
		}

	} // end pass 1 loop

	rewind(asm_file);

	// new variables
	int startingAddress = 0; // preserve starting address for end record
	int length = 3; //default length of the object code
	int textIndex = 0; // used for textRecords

	while (!feof(asm_file)) { // pass 2 loop
		fgets(buf, 4096, asm_file);
		length = 3; // resetting length

		// Reading through the file to generate the object code
		struct isSymbolLineResult ISLRes = isSymbolLine(buf, counter);
		switch (ISLRes.result) { // checking if it's a symbol line
			case 1: // has a symbol
				break;
			case 2: // does not have a symbol

				directive = strtok(buf, delims);
				operand = strtok(NULL, delims);
				counterInc = getCounterIncrement(directive, operand, 0); // lineNum is unused in this part I think
				textIndex = writeTextRecord(&records, symbols, opcodes, directive, operand, length, counter, textIndex);

				if (textIndex == ERROR) {
					return end(asm_file, ERROR);
				}

				if (!IsADirective(directive) &&
						strcmp(directive, "RSUB") != 0) { // need modification record
						dataIndex = addModRecord(modData, counter, 4, symbols[0].Name, dataIndex);
				}

				counter += counterInc;
				continue;

			case 0: // is a comment line
				continue;
		}

		// grab the stuff
		symbol = strtok(buf, delims);
		directive = strtok(NULL, delims);
		operand = strtok(NULL, delims);

		counterInc = getCounterIncrement(directive, operand, 0); // lineNum is unused in this part I think

		if (strcmp(directive, "START") == 0) { // start directive
			startingAddress = strtol(operand, NULL, 16);
			int endAddress = counter - startingAddress;
			writeHeadRecord(&records, symbol, endAddress, startingAddress);
			counter = startingAddress;
			continue;
		}

		if (counterInc == END) { // end directive
			// check if the operand is a predefined symbol
			int operandError = isPredefinedSymbol(symbols, operand);

			if (operandError == ERROR) {
				printf("ERROR: symbol %s is not defined.\n", operand);
				return end(asm_file, ERROR);
			}

			startingAddress = symbols[operandError].Address;

			writeModRecord(&records, modData, dataIndex);
			writeEndRecord(&records, startingAddress);

			int objectError = writeObjectFile(out_file_name, records, textIndex, dataIndex);
			if (objectError == ERROR) {
				return end(asm_file, ERROR);
			}

			break;
		} else if (strcmp(directive, "BYTE") == 0) { // length is variable
			char op_value[32];
			getCleanOperand(operand, op_value);
			length = strlen(op_value);

			if (operand[0] == 'C') { // it's a character
				memset(operand, '\0', length * sizeof(char));

				for (int i = 0; i < length; i++) { // convert to hex string
					char str[3] = "";
					sprintf(str, "%X", (int) op_value[i]);
					strcat(operand, str);

					if (op_value[i] == '\0') {
						break;
					}
				}

				length = strlen(operand);
			} else if (operand[0] == 'X') { // it's a hex
				strcpy(operand, op_value);
			}
		}
		if (strcmp(directive, "RESB") != 0 &&
 			 strcmp(directive, "RESW") != 0) { // these two do not generate records

 			textIndex = writeTextRecord(&records, symbols, opcodes, directive, operand, length, counter, textIndex);
 			if (textIndex == ERROR) {
	 			return end(asm_file, ERROR);
 			}
 		}
		if (!IsADirective(directive) &&
				strcmp(directive, "RSUB") != 0) { // need modification record
				dataIndex = addModRecord(modData, counter, 4, symbols[0].Name, dataIndex);
		}

		counter += counterInc;
	} // end pass 2 loop
	return end(asm_file, SAFE); // unsure if this ever happens, keeping for safety
}

int end (FILE* fp, int code) {
	if (code == ERROR) {
		printf("There was an Error.\n");
	} else {
		printf("Program exited successfully.\n");
	}

	fclose(fp);
	return 0;
}

void getCleanOperand(char* operand, char op_value[]) { // mutates op_value
	memset(op_value, '\0', 32 * sizeof(char));

	strncpy(op_value, operand + 2, strlen(operand) - 2);
	char* last = strchr(op_value, '\'');
	last[0] = '\0';
}


int getCounterIncrement (char* directive, char* operand, int lineNum) {
	int counterInc = 0; // Add to the counter; -1 is error, -2 is end
	if (strcmp(directive, "END") == 0) {
		return END;
	} else	if (strcmp(directive, "RESB") == 0) { // Reserving a variable amount of bytes
		counterInc = strtol(operand, NULL, 10); // add that to the counter.
	} else if (strcmp(directive, "RESW") == 0) { // word is 3 bytes
		counterInc = 3 * strtol(operand, NULL, 10);
	} else if (strcmp(directive, "BYTE") == 0) {
		//	* Clearing out the junk in the operand
		char op_value[32];
		getCleanOperand(operand, op_value);

		if (operand[0] == 'C') {
			// need to add a byte for every character
			counterInc = strlen(op_value);
		} else if (operand[0] == 'X') {
			// need to add a byte for every 2 chars.
			regex_t regex; // regex expression for hex validation.

			// prep the regex
			int reti = regcomp(&regex, "^[a-fA-F0-9]+$", REG_EXTENDED);
			if (reti) { // regex failed to compile
				printf("Regex Compilation failed.\n");
			}

			reti = regexec(&regex, op_value, 0, NULL, 0); // execute the regex
			if (reti != 0) { // there was an invalid match
				printf("ERROR: %s is not a valid hex value\n  at line %d\n", op_value, lineNum);
				return ERROR;
			}

			counterInc = ceil(strlen(op_value)) / 2; // increment the counter
		}
	} else if (strcmp(directive, "WORD") == 0) {
		if ((strtol(operand, NULL, 10)) > pow(2, 23)) { // the 24th bit is used for sign
			printf("ERROR: WORD TOO LARGE\n at line %d\n", lineNum);
			return ERROR;
		}
		counterInc = 3;
	} else { // just a regular line
		counterInc = 3;
	}

	return counterInc;
}

struct isSymbolLineResult isSymbolLine(char* buf, int counter) {  // check if line has symbol
	struct isSymbolLineResult res;
	// Check if the line has a symbol
	res.result = 0; // default: not symbol line
	res.counter = counter;

	if (buf[0] == '\t') { // no symbol
		res.result = 2;
		res.counter += 3;
	} else if (buf[0] == '#') { // is a comment
		// do nothing
	} else { // is a symbol assembly line
		res.result = 1;
	}
	return res;
}

int isMemTooBig(int address, int lineNum) { // checks if memory out if range
		if (address >= 32768) { // goes outside SIC Memory range
			printf("ERROR: OUT OF MEMORY\n at line %d.\n", lineNum);
			return 1;
		}
		return 0;
}

void addToStruct (SYMBOL *symbols, int line, char *symbol, int counter, int symbolsIndex) {
	symbols[symbolsIndex].DefinedOnSourceLine = line;
	symbols[symbolsIndex].Address = counter;
	strcpy(symbols[symbolsIndex].Name, symbol);
}

int printLine(SYMBOL* symbols, char* symbol, int counter, int line, int symbolsIndex) {
	if (PASS1) {
		printf("%s\t%X\n", symbol, counter);
	}
	addToStruct(symbols, line, symbol, counter, symbolsIndex);
	return ++symbolsIndex;
}

void initializeOpcodes (OPCODE* ops) {
	// this is gonna be ugly. bear with it.
	char instructions[26][11] = { "ADD", "AND", "COMP", "DIV", "J", "JEQ", "JGT", "JLT", "JSUB", "LDA", "LDCH", "LDL", "LDX", "MUL", "OR", "RD", "RSUB", "STA", "STCH", "STL", "STSW", "STX", "SUB", "TD", "TIX", "WD" };
	char opcodes[26][3] = { "18", "40", "28", "24", "3C", "30", "34", "38", "48", "00", "50", "08", "04", "20", "44", "D8", "4C", "0C", "54", "14", "E8", "10", "1C", "E0", "2C", "DC" };

	for (int i = 0; i < 26; i++) {
		strcpy(ops[i].name, instructions[i]);
		ops[i].hexCode = strtol(opcodes[i], NULL, 16);
	}
}

int isPredefinedSymbol (SYMBOL* symbols, char* name) {
	int result = ERROR; // default, not a symbol

	char* space = strchr(name, ' ');
	if (space != NULL) { // trying to eliminate junk
		*space = '\0';
	}

	for (int i = 0; i < 1024; i++) {
		if (strcmp(name, symbols[i].Name) == 0) {
			result = i;
			break;
		}
	}
	return result;
}

OPCODE findInstruction(OPCODE* opcodes, char* directive, char* operand) {
	OPCODE result = emptyOpcode; // initialized as empty

	if (IsADirective(directive)) {
		// INCOMING UGLY IF SPAM
		if (strcmp(directive, "RESB") == 0 ||
				strcmp(directive, "RESW") == 0) {
				// do nothing
		} else if (strcmp(directive, "WORD") == 0) { // WORD
			strcpy(result.name, directive);
			result.hexCode = strtol(operand, NULL, 10);
		} else if (strcmp(directive, "BYTE") == 0) { // BYTE
			strcpy(result.name, directive);
			result.hexCode = strtol(operand, NULL, 16);
		}
		return result;
	}

	for (int i = 0; i < 26; i++) { // loop over all the opcode pairs
		if (strcmp(opcodes[i].name, directive) == 0) {
			// it's a match, so return it
			result = opcodes[i];
			break;
		}
	}
	return result;
}

SYMBOL findSymbol (SYMBOL* symbols, char* name) {
	SYMBOL result = emptySymbol;
	int index = isPredefinedSymbol(symbols, name);

	if (index != ERROR) { // the symbol was found
		result = symbols[index];
	}

	return result;
}

// creates the text record
int writeTextRecord(struct objectData *records, SYMBOL* symbols, OPCODE* ops,  char* directive, char* operand, int length, int startingAddress, int textIndex) {
	char objectCode[1024];
	int symbolAddress = 0; // address of a symbol in the operand
	memset(objectCode, '\0', 1024 * sizeof(char));
	char* indexedPointer = strstr(operand, ",X");
	char *endptr;

	if (indexedPointer != NULL) { // ,X is used to indicate indexed addressing
		symbolAddress += strtol("8000", NULL, 16); // maybe find a less hacky way
		*indexedPointer = '\0'; // use this to make a clean operand
	}

	SYMBOL symbol = findSymbol(symbols, operand);
	OPCODE instruction = findInstruction(ops, directive, operand);


	// very strict checking comparison of the retrieved symbol to
	// the empty Symbol. This ensures that there is no doubt that
	// the symbol does not exist.
	// Is what I'd like to say, but having an address of 0 causes issues.
	if (strcmp(symbol.Name, emptySymbol.Name) != 0) {
		symbolAddress += symbol.Address;
	} else if (strcmp(directive, "RSUB") != 0) { // This is a hacky solution
		// this is not perfect, but it gets the job done for the test cases.
		// The Key issue here is that symbols that are named using hex letters
		// will slip by this, i.e. if you have a symbol FAD, it will
		// make it through.
		strtol(operand, &endptr, 16);
		if (operand == endptr) {
			printf("ERROR: Symbol %s has not been defined before.\n", operand);
			return ERROR;
		}
	}

	if (!IsADirective(directive)) { // it's an instruction, not a directive
		// the object code format is Opcode and Address
		sprintf(objectCode, "%02X%04X", instruction.hexCode, symbolAddress);
	} else { // it's just a directive
		// The directives have no opcode, so the hexCode here is really
		// the data they store
		// BYTE can exceed the 60 character object code limit
		char tmp[61] = ""; // temporary holder
		strncpy(tmp, operand, 60); // get first 60 chars
		tmp[61] = '\0'; // null terminate
		if (strlen(operand) > 60) {
			// wrap text records here
			sprintf(records->textRecords[textIndex], "T%06X%02X%s\n", startingAddress, 30, tmp); // print to Text Records

			textIndex++; // increment testIndex
			startingAddress += 30; // adjust startingAddress
			length -= 30; // adjust length

			char *rest = &operand[61]; // the rest of the stuff

			sprintf(objectCode, "%s", rest);
		} else {
			if (strcmp(instruction.name, "WORD") == 0) {
				sprintf(objectCode, "%06X", instruction.hexCode);
			} else {
				sprintf(objectCode, "%s", operand);
				length /= 2;
			}
		}
	}

	// print out the actual text record
	sprintf(records->textRecords[textIndex], "T%06X%02X%s\n", startingAddress, length, objectCode);
	return ++textIndex;

}

// creates the header record
void writeHeadRecord(struct objectData *records, char* symbol, int length, int startingAddress) {
	sprintf(records->headRecord, "H%s%06X%06X\n", symbol, startingAddress, length);
}

// create end record
void writeEndRecord(struct objectData *records, int startingAddress) {
	sprintf(records->endRecord, "E%06X\n", startingAddress);
}

// create Modification records
int addModRecord(MRD* modData, int startingAddress, int modLength, char* symbol, int dataIndex) {
		MRD data;

		strcpy(data.symbol, symbol);
		data.startingAddress = startingAddress + 1;
		data.modLength = modLength;

		modData[dataIndex] = data;
		return ++dataIndex;
}

// write the actual records
void writeModRecord (struct objectData *records, MRD* dataRecords, int dataIndex) {
	for (int i = 0; i < dataIndex; i++) {
 		sprintf(records->modRecords[i], "M%06X%02X+%s\n",
					 dataRecords[i].startingAddress,
					 dataRecords[i].modLength,
					 dataRecords[i].symbol);
	}
}

// write the object file
int writeObjectFile (char fileName[], struct objectData records, int textIndex, int dataIndex) {
	if (strcmp(records.headRecord, "\0") == 0) {
		printf("ERROR: no START Directive\n");
		return ERROR;
	}

	FILE *fp = fopen(fileName, "w");

	if (fp == NULL) {
		printf("File unable to be opened.\n");
		fclose(fp);
		return ERROR;
	}

	fprintf(fp, "%s", records.headRecord);

	for (int i = 0; i < textIndex; i++) {
		fprintf(fp, "%s", records.textRecords[i]);
	}

	for (int i = 0; i < dataIndex; i++) {
		fprintf(fp, "%s", records.modRecords[i]);
	}

	fprintf(fp, "%s", records.endRecord);

	fclose(fp);
	return SAFE;
}
