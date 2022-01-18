#include "headers.h"

int IsAValidSymbol( char *TestSymbol, char *prevSymbols ){ // Testing if TestSymbol is a valid symbol
/********
 * What makes a Valid Symbol? Elementary my dear Watson
 * Cannot be named the same as a directive
 * Must start with Alpha (although can contain numbers after the first character)
 * Cannot be longer than 6 characters
 * cannot contain spaces, $, !, =, +, -, (, ), or @
 * Although I guess other special characters can be used?
*/
	int result = 1; // I don't believe in capitalized variables
	int maxlen = 6;
	int index = 0;
	int done = 0;
	char badChars[] = " $!=+-()@";
	while (!done) { // the validation loop
		if (strstr(prevSymbols, TestSymbol) != NULL) { // check if the symbol has been used
			result = 0;
			break;
		}
		if (strpbrk(TestSymbol, badChars) != NULL) { // contains badChars
			result = 0;
			break;
		}
		if (IsADirective(TestSymbol) != 0) { // Is a directive
			result = 0;
			break;
		}
		if (isalpha(TestSymbol[0]) == 0 && isupper(TestSymbol[0]) == 0) { // Does not start with alpha
			result = 0;
			break;
		}
		if (TestSymbol[index] == NULL) { // There is nothing left
			done = 1;
			break;
		}

		if (index == maxlen) { // longer than six characters
			result = 0;
			done = 1;
			break;
		}
		index++;
	}

	strcat(prevSymbols, TestSymbol);

	return result;
}
