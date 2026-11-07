// Luis Adan Arreola
// CS 4301
// Stage 1

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <ctime>
#include <cctype>
#include "stage1.h"
#include <algorithm>
#include <cstdio>
#include <sstream>
#include <iomanip>

using namespace std;

// Global Variable(s)
int labelCounter = 0;

typedef map<string, SymbolTableEntry> SymTab;

/* ******************************************************************************************************************************* */
// Lexicon helpers

bool Compiler::isKeyword(string s) const
{
	// List of keywords
	vector<string> keywords =
	{
		"program", "begin", "end", "var", "const",
		"integer", "boolean", "true", "false", "not",
		"mod", "div", "and", "or", "read", "write",
		"if", "then", "else", "repeat", "while", "do", "until"
	};

	for (const string& keyword : keywords)
	{
		if (s == keyword)
			return true; // Found a match
	}

	// no match found
	return false;
}

bool Compiler::isSpecialSymbol(char c) const
{
	const string specialSymbols = "=:,;.+-*<>()";

	// Check if c is a special symbol
	for (char symbol : specialSymbols)
	{
		if (c == symbol)
			return true;
	}

	return false;
}

bool Compiler::isInteger(string s) const
{
	if (s.empty())
		return false;

	size_t i = 0;

	// An optional sign must be followed by at least one digit
	if (s[0] == '+' || s[0] == '-')
		i = 1;

	if (i >= s.length())
		return false;

	for (; i < s.length(); ++i)
	{
		if (!isdigit(static_cast<unsigned char>(s[i])))
			return false;
	}

	return true;
}

bool Compiler::isBoolean(string str) const
{
	return str == "true" || str == "false";
}

bool Compiler::isNonKeyId(string s) const
{
	// A non-key id starts with a lowercase letter and continues with
	// lowercase letters, digits, or underscores
	if (s.empty() || !islower(static_cast<unsigned char>(s[0])))
		return false;

	for (char c : s)
	{
		unsigned char u = static_cast<unsigned char>(c);

		if (!islower(u) && !isdigit(u) && c != '_')
			return false;
	}

	return !isKeyword(s);
}

bool Compiler::isLiteral(string s) const
{
	return isInteger(s) || isBoolean(s);
}

string Compiler::genInternalName(storeTypes stype) const
{
	static int booleans = 0;
	static int integers = 0;
	static int unknowns = 0;

	string internName;

	switch(stype)
	{
		case PROG_NAME:
			internName = "P0";
			break;

		case INTEGER:
			internName = "I" + to_string(integers);
			integers++;
			break;

		case BOOLEAN:
			internName = "B" + to_string(booleans);
			booleans++;
			break;

		case UNKNOWN:
			internName = "U" + to_string(unknowns);
			unknowns++;
			break;

		default:
			internName = "X" + to_string(unknowns);
			unknowns++;
			break;
	}

	return internName;
}

// Operator classes used by the expression productions
static bool isRelationalOperator(const string& s)
{
	return s == "=" || s == "<>" || s == "<=" || s == ">=" || s == "<" || s == ">";
}

static bool isAddLevelOperator(const string& s)
{
	return s == "+" || s == "-" || s == "or";
}

static bool isMultLevelOperator(const string& s)
{
	return s == "*" || s == "div" || s == "mod" || s == "and";
}

// Can token t be the first token of an expression?
static bool startsExpression(const Compiler& c, const string& t)
{
	return t == "not" || t == "(" || t == "+" || t == "-" ||
	       c.isInteger(t) || c.isBoolean(t) || c.isNonKeyId(t);
}

/* ******************************************************************************************************************************* */

Compiler::Compiler(char **argv)
{
	// Check if the correct number of command-line arguments is provided
	if (argv[1] == nullptr || argv[2] == nullptr || argv[3] == nullptr)
	{
		cerr << "Usage: ./compiler input_file listing_file object_file" << endl;
		exit(EXIT_FAILURE);
	}

	// Open the sourceFile using argv[1]
	sourceFile.open(argv[1]);
	if (!sourceFile.is_open())
	{
		cerr << "Error: Unable to open input file " << argv[1] << endl;
		exit(EXIT_FAILURE);
	}

	// Open the listingFile using argv[2]
	listingFile.open(argv[2]);

	if (!listingFile.is_open())
	{
		cerr << "Error: Unable to open listing file " << argv[2] << endl;
		sourceFile.close();
		exit(EXIT_FAILURE);
	}

	// Open the objectFile using argv[3]
	objectFile.open(argv[3]);
	if (!objectFile.is_open())
	{
		cerr << "Error: Unable to open object file " << argv[3] << endl;

		// Close the source and listing file
		sourceFile.close();
		listingFile.close();
		exit(EXIT_FAILURE);
	}
}

Compiler::~Compiler() // destructor
{
	// close all open files
	sourceFile.close();
	listingFile.close();
	objectFile.close();
}

void Compiler::createListingHeader()
{
	//line numbers and source statements should be aligned under the headings
	time_t now = time (NULL);

	listingFile << "STAGE1:  " << "   Luis Arreola & Htoo Myat       " <<  ctime(&now) << endl;
	listingFile << "LINE NO.              SOURCE STATEMENT" << endl << endl;
}

void Compiler::parser()
{
	nextChar();

	if (nextToken() != "program")
		processError("keyword 'program' expected");

	prog();
}

void Compiler::createListingTrailer()
{
	listingFile << "\n" << "COMPILATION TERMINATED      " << errorCount << " ERRORS ENCOUNTERED" << endl;
}

void Compiler::processError(string err)
{
	errorCount++;

	//Output err to listingFile
	listingFile << endl << endl << "Error: Line " << lineNo << ": " << err << endl;

	listingFile << "\n" << "COMPILATION TERMINATED      " << errorCount << " ERROR ENCOUNTERED" << endl;

	// exit() does not run the destructor of a local Compiler object,
	// so make sure everything written so far reaches the files
	listingFile.flush();
	objectFile.flush();

	//Call exit() to terminate program
	exit(EXIT_FAILURE);
}

/* ******************************************************************************************************************************* */
// Stage 0 productions

void Compiler::prog() //token should be "program"
{
	if (token != "program")
		processError("keyword 'program' expected");

	progStmt();

	if (token == "const")
		consts();

	if (token == "var")
		vars();

	if (token != "begin")
		processError("keyword 'begin' expected");

	beginEndStmt();

	if (token[0] != END_OF_FILE)
		processError("no text may follow 'end'");
}

void Compiler::progStmt() //token should be "program"
{
	string x;

	if (token != "program")
		processError("keyword program expected");

	x = nextToken();

	if (!isNonKeyId(token))
		processError("program name expected");

	if (nextToken() != ";")
		processError("semicolon expected");

	nextToken();
	insert(x, PROG_NAME, CONSTANT, x, NO, 0);
	code("program", x);
}

void Compiler::consts() //token should be "const"
{
	if (token != "const")
		processError("keyword 'const' expected");

	if (!isNonKeyId(nextToken()))
		processError("non-keyword identifier must follow 'const'");

	constStmts();
}

void Compiler::vars() // token should be "var"
{
	if (token != "var")
		processError("keyword 'var' expected");

	// Move to the next token after "var"
	nextToken();

	// Check if the next token is a non-keyword identifier
	if (!isNonKeyId(token))
		processError("non-keyword identifier must follow 'var'");

	varStmts();
}

void Compiler::beginEndStmt() //token should be "begin"
{
	if (token != "begin")
		processError("keyword 'begin' expected");

	if (isNonKeyId(nextToken()) || token == "read" || token == "write")
		execStmts();

	if (token != "end")
		processError("non-keyword identifier, 'read', 'write', or 'end' expected");

	if (nextToken() != ".")
		processError("period expected after keyword 'end'");

	code("end", "", "");
	nextToken();
}

void Compiler::constStmts() //token should be NON_KEY_ID
{
	string x, y;

	if (!isNonKeyId(token))
		processError("non-keyword identifier expected");

	x = token;

	if (nextToken() != "=")
		processError("'=' expected");

	y = nextToken();

	if (y != "+" && y != "-" && y != "not" && !isNonKeyId(y) && !isInteger(y) && !isBoolean(y))
		processError("token to right of '=' illegal");

	if (y == "+" || y == "-")
	{
		if (!isInteger(nextToken()))
			processError("integer expected after sign");

		// a leading '+' is simply dropped
		y = (y == "-") ? "-" + token : token;
	}

	if (y == "not")
	{
		if (nextToken() != "true" && token != "false" && !isNonKeyId(token))
			processError("boolean expected after not");

		if (token == "true")
			y = "false";

		else if (token == "false")
			y = "true";

		else
		{
			if (whichType(token) != BOOLEAN)
				processError("boolean expected after not");

			// booleans are stored as -1 (true) and 0 (false)
			y = (whichValue(token) == "-1") ? "false" : "true";
		}
	}

	if (isNonKeyId(y))
	{
		// Check if y is in the symbol table and if its type is not PROG_NAME.
		auto SymbolI = symbolTable.find(y);

		if (SymbolI != symbolTable.end() && SymbolI->second.getDataType() == PROG_NAME)
			processError("data type of token on the right-hand side must be INTEGER or BOOLEAN");
	}

	if (nextToken() != ";")
		processError("semicolon expected");

	insert(x, whichType(y), CONSTANT, whichValue(y), YES, 1);

	if (nextToken() != "begin" && token != "var" && !isNonKeyId(token))
		processError("non-keyword identifier, 'begin', or 'var' expected");

	if (isNonKeyId(token))
		constStmts();
}

void Compiler::varStmts() //token should be NON_KEY_ID
{
	string x, y;

	if (!isNonKeyId(token))
		processError("non-keyword identifier expected");

	// Retrieve a comma-separated list of variable names
	x = ids(); // ids() leaves 'token' on the token after the identifier list

	if (token != ":")
		processError("\":\" expected");

	// Get the next token, which should specify the type
	nextToken();

	if (token != "integer" && token != "boolean")
		processError("illegal type follows \":\"");

	y = token;

	if (nextToken() != ";")
		processError("semicolon expected after type specifier");

	// Insert the variables into the symbol table with their type
	if (y == "integer")
		insert(x, INTEGER, VARIABLE, "", YES, 1);

	else
		insert(x, BOOLEAN, VARIABLE, "", YES, 1);

	// Move to the next token after the semicolon
	nextToken();

	// Expecting either another variable declaration or the 'begin' keyword
	if (token != "begin" && !isNonKeyId(token))
		processError("non-keyword identifier or 'begin' expected after variable declaration");

	if (isNonKeyId(token))
		varStmts();
}

string Compiler::ids() //token should be NON_KEY_ID
{
	string temp, tempString;

	if (!isNonKeyId(token))
		processError("non-keyword identifier expected");

	tempString = token;
	temp = token;

	if (nextToken() == ",")
	{
		if (!isNonKeyId(nextToken()))
			processError("non-keyword identifier expected");

		tempString = temp + "," + ids();
	}

	return tempString;
}

/* ******************************************************************************************************************************* */
// Action routines

void Compiler::insert(string externalName, storeTypes inType, modes inMode, string inValue, allocation inAlloc, int inUnits)
{
	// Split the externalName string by commas to handle multiple names
	stringstream ss(externalName);
	string name;

	while (getline(ss, name, ','))
	{
		name.erase(remove_if(name.begin(), name.end(), ::isspace), name.end());

		// Check for multiple definitions or illegal use of keywords. The boolean
		// literals true and false are keywords, but they get symbol table entries
		// of their own when they are used as operands.
		if (symbolTable.find(name) != symbolTable.end())
			processError("multiple name definition for: " + name);

		else if (isKeyword(name) && !isBoolean(name))
			processError("illegal use of keyword: " + name);

		else
		{
			// Names that start with an uppercase letter (the temporaries T0, T1, ...)
			// keep their name; everything else gets a generated internal name
			string internalName = isupper(static_cast<unsigned char>(name[0])) ? name : genInternalName(inType);

			symbolTable.emplace(name, SymbolTableEntry(internalName, inType, inMode, inValue, inAlloc, inUnits));
		}
	}
}

storeTypes Compiler::whichType(string name) //tells which data type a name has
{
	if (isBoolean(name))
		return BOOLEAN;

	else if (isLiteral(name))
		return INTEGER;

	else
	{
		auto i = symbolTable.find(name);

		if (i != symbolTable.end())
			return i->second.getDataType();

		processError("reference to undefined symbol: " + name);
		return UNKNOWN;
	}
}

string Compiler::whichValue(string name) //tells which value a name has
{
	if (isBoolean(name))
	{
		if (name == "true")
			return "-1";
		else
			return "0";
	}

	else if (isInteger(name))
		return name;

	// Name is an identifier and hopefully a constant
	auto i = symbolTable.find(name);

	if (i != symbolTable.end() && i->second.getMode() == CONSTANT)
		return i->second.getValue();

	processError("reference to undefined constant: " + name);
	return "";
}

void Compiler::code(string op, string operand1, string operand2)
{
	if (op == "program")
		emitPrologue(operand1);

	else if (op == "end")
		emitEpilogue();

	else if (op == "read")
		emitReadCode(operand1);

	else if (op == "write")
		emitWriteCode(operand1);

	else if (op == "+")				// binary '+'
		emitAdditionCode(operand1, operand2);

	else if (op == "-")				// binary '-'
		emitSubtractionCode(operand1, operand2);

	else if (op == "neg")			// unary '-'
		emitNegationCode(operand1);

	else if (op == "not")
		emitNotCode(operand1);

	else if (op == "*")
		emitMultiplicationCode(operand1, operand2);

	else if (op == "div")
		emitDivisionCode(operand1, operand2);

	else if (op == "mod")
		emitModuloCode(operand1, operand2);

	else if (op == "and")
		emitAndCode(operand1, operand2);

	else if (op == "or")
		emitOrCode(operand1, operand2);

	else if (op == "=")
		emitEqualityCode(operand1, operand2);

	else if (op == "<>")
		emitInequalityCode(operand1, operand2);

	else if (op == "<")
		emitLessThanCode(operand1, operand2);

	else if (op == "<=")
		emitLessThanOrEqualToCode(operand1, operand2);

	else if (op == ">")
		emitGreaterThanCode(operand1, operand2);

	else if (op == ">=")
		emitGreaterThanOrEqualToCode(operand1, operand2);

	else if (op == ":=")
		emitAssignCode(operand1, operand2);

	else
		processError("compiler error since function code should not be called with illegal arguments");
}

string Compiler::getLabel()
{
	labelCounter++;

	return "L" + to_string(labelCounter);
}

void Compiler::freeTemp()
{
	currentTempNo--;

	if (currentTempNo < -1)
		processError("compiler error, currentTempNo should be >= -1");
}

string Compiler::getTemp()
{
	string temp;
	currentTempNo++;

	temp = "T" + to_string(currentTempNo);

	if (currentTempNo > maxTempNo)
	{
		insert(temp, UNKNOWN, VARIABLE, "", NO, 1);
		maxTempNo++;
	}

	return temp;
}

bool Compiler::isTemporary(string s) const
{
	// A temporary is the letter 'T' followed by one or more digits
	if (s.length() < 2 || s[0] != 'T')
		return false;

	for (size_t i = 1; i < s.size(); ++i)
	{
		if (!isdigit(static_cast<unsigned char>(s[i])))
			return false;
	}

	return true;
}

/* ******************************************************************************************************************************* */
// Stage 1 productions
//
// Every production follows the same rule: when it is called, 'token' is the first
// token of the production, and when it returns, 'token' is the first token after it.
//
// Operand order for the code generator: operand1 is the RIGHT operand (popped first)
// and operand2 is the LEFT operand, so  code(op, operand1, operand2)  means  op2 op op1.

void Compiler::execStmts()      // stage 1, production 2
{
	// As long as the current token begins an executable statement, process it
	while (isNonKeyId(token) || token == "read" || token == "write")
		execStmt();
}

void Compiler::execStmt()       // stage 1, production 3
{
	if (isNonKeyId(token))
		assignStmt();

	else if (token == "read")
		readStmt();

	else if (token == "write")
		writeStmt();

	else
		processError("non-keyword identifier, 'read', or 'write' expected");
}

void Compiler::assignStmt()     // stage 1, production 4
{
	if (!isNonKeyId(token))
		processError("non-keyword identifier expected for assignment");

	if (symbolTable.find(token) == symbolTable.end())
		processError("reference to undefined variable: " + token);

	pushOperand(token);

	if (nextToken() != ":=")
		processError("':=' expected in assignment statement");

	pushOperator(":=");
	nextToken();
	express();

	if (token != ";")
		processError("one of \"*\", \"and\", \"div\", \"mod\", \"+\", \"-\", \"or\", \"=\", \"<>\", \"<=\", \">=\", \"<\", \">\", or \";\" expected");

	string operand1 = popOperand();		// right-hand side
	string operand2 = popOperand();		// left-hand side
	code(popOperator(), operand1, operand2);

	nextToken();
}

void Compiler::readStmt()       // stage 1, production 5
{
	if (nextToken() != "(")
		processError("'(' expected after 'read'");

	nextToken();
	string idList = ids();

	if (token != ")")
		processError("',' or ')' expected after non-keyword identifier");

	code("read", idList);

	if (nextToken() != ";")
		processError("';' expected after 'read' statement");

	nextToken();
}

void Compiler::writeStmt()      // stage 1, production 7
{
	if (nextToken() != "(")
		processError("'(' expected after 'write'");

	nextToken();
	string idList = ids();

	if (token != ")")
		processError("',' or ')' expected after non-keyword identifier");

	code("write", idList);

	if (nextToken() != ";")
		processError("';' expected after 'write' statement");

	nextToken();
}

void Compiler::express()        // stage 1, production 9
{
	if (!startsExpression(*this, token))
		processError("'not', '(', '+', '-', boolean literal, integer literal, or non-keyword identifier expected");

	term();
	expresses();
}

void Compiler::expresses()      // stage 1, production 10
{
	// relational operators have the lowest precedence
	while (isRelationalOperator(token))
	{
		pushOperator(token);
		nextToken();
		term();

		string operand1 = popOperand();
		string operand2 = popOperand();
		code(popOperator(), operand1, operand2);
	}
}

void Compiler::term()           // stage 1, production 11
{
	if (!startsExpression(*this, token))
		processError("'not', '(', '+', '-', boolean literal, integer literal, or non-keyword identifier expected");

	factor();
	terms();
}

void Compiler::terms()          // stage 1, production 12
{
	// additive level: + - or   (left associative)
	while (isAddLevelOperator(token))
	{
		pushOperator(token);
		nextToken();
		factor();

		string operand1 = popOperand();
		string operand2 = popOperand();
		code(popOperator(), operand1, operand2);
	}
}

void Compiler::factor()         // stage 1, production 13
{
	if (!startsExpression(*this, token))
		processError("'not', '(', '+', '-', boolean literal, integer literal, or non-keyword identifier expected");

	part();
	factors();
}

void Compiler::factors()        // stage 1, production 14
{
	// multiplicative level: * div mod and   (left associative)
	while (isMultLevelOperator(token))
	{
		pushOperator(token);
		nextToken();
		part();

		string operand1 = popOperand();
		string operand2 = popOperand();
		code(popOperator(), operand1, operand2);
	}
}

void Compiler::part()           // stage 1, production 15
{
	string operand;

	if (token == "not")
	{
		nextToken();
		part();
		code("not", popOperand());
	}

	else if (token == "+")
	{
		nextToken();

		if (isInteger(token))
		{
			pushOperand(token);
			nextToken();
		}

		else
		{
			// unary '+' does not change the value, but the operand must be an integer
			part();
			operand = popOperand();

			if (whichType(operand) != INTEGER)
				processError("unary '+' requires an integer operand");

			pushOperand(operand);
		}
	}

	else if (token == "-")
	{
		nextToken();

		if (isInteger(token))
		{
			// a negative integer literal such as -5
			pushOperand("-" + token);
			nextToken();
		}

		else
		{
			part();
			code("neg", popOperand());
		}
	}

	else if (token == "(")
	{
		nextToken();
		express();

		if (token != ")")
			processError("')' expected");

		nextToken();
	}

	else if (isInteger(token) || isBoolean(token))
	{
		pushOperand(token);
		nextToken();
	}

	else if (isNonKeyId(token))
	{
		auto entry = symbolTable.find(token);

		if (entry == symbolTable.end())
			processError("reference to undefined variable or constant: " + token);

		if (entry->second.getDataType() == PROG_NAME)
			processError("the program name cannot be used in an expression");

		// constants and variables are both referenced through their own storage
		pushOperand(token);
		nextToken();
	}

	else
		processError("'not', '(', '+', '-', boolean literal, integer literal, or non-keyword identifier expected");
}

/* ******************************************************************************************************************************* */
// Lexical routines

string Compiler::nextToken() //returns the next token or end of file marker
{
	token = "";

	while (token == "")
	{
		if (ch == '{')
		{
			// Process comments
			while (nextChar() != '}')
			{
				if (ch == END_OF_FILE)
				{
					processError("unexpected end of file");
					return token;
				}
			}
			nextChar(); // Move past the closing '}'
		}

		else if (ch == '}')
		{
			processError("'}' cannot begin a token");
			nextChar();
		}

		else if (isspace(static_cast<unsigned char>(ch)))
			nextChar();

		else if (isdigit(static_cast<unsigned char>(ch)))
		{
			token += ch;

			while (isdigit(static_cast<unsigned char>(nextChar())) || ch == '.')
				token += ch;

			// After constructing the token, check if it's a valid integer
			if (!isInteger(token))
				processError("no real numbers are allowed");
		}

		else if (isSpecialSymbol(ch))
		{
			token = ch;
			nextChar();

			if (token == "<")
			{
				if (ch == '=' || ch == '>')
				{
					token += ch;
					nextChar();
				}
			}

			else if (token == ">" || token == ":")
			{
				if (ch == '=')
				{
					token += ch;
					nextChar();
				}
			}
		}

		else if (islower(static_cast<unsigned char>(ch)))
		{
			token += ch;

			while (islower(static_cast<unsigned char>(nextChar())) || isdigit(static_cast<unsigned char>(ch)) || ch == '_')
			{
				if (token.back() == '_' && ch == '_')
				{
					processError("encountered consecutive underscores");
					return token;
				}
				token += ch;
			}

			if (token.back() == '_')
			{
				processError("'_' cannot end a token");
				return token;
			}
		}

		else if (ch == END_OF_FILE)
			token = ch;

		else
			processError("illegal symbol");
	}

	return token;
}

char Compiler::nextChar() //returns the next character or end of file marker
{
	sourceFile.get(ch);

	static char oldChar = '\n';

	if (sourceFile.eof())
	{
		ch = END_OF_FILE;
		return ch;
	}

	else
	{
		if (oldChar == '\n')
			listingFile << setw(5) << ++lineNo << '|';

		listingFile << ch;
	}

	oldChar = ch;
	return ch;
}

/* ******************************************************************************************************************************** */
// Emit routines

void Compiler::emit(string label, string instruction, string operands, string comment)
{
	// Turn on left justification in objectFile
	objectFile << left;

	// Output label in a field of width 8
	objectFile << setw(8) << label;

	// Output instruction in a field of width 8
	objectFile << setw(8) << instruction;

	// Output the operands in a field of width 24
	objectFile << setw(24) << operands;

	// Output the comment
	objectFile << comment << endl;
}

void Compiler::emitPrologue(string progName, string operand2)
{
	time_t now = time (NULL);
	objectFile << ";    Luis Arreola & Htoo Myat     " << ctime(&now);

	// Output %INCLUDE'S
	objectFile << "%INCLUDE \"Along32.inc\"" << endl;
	objectFile << "%INCLUDE \"Macros_Along.inc\"" << endl << endl;

	// Emit .text section
	emit("SECTION", ".text");

	// made sure the program name is not longer then 15 characters
	string newProgName = progName.substr(0, 15);

	// Emit the label for the start of the program
	emit("global", "_start", "", "; program " + newProgName);

	objectFile << endl;

	// Emit _start
	emit("_start:");
}

void Compiler::emitEpilogue(string operand1, string operand2)
{
	emit("", "Exit", "{0}");
	emitStorage();
}

void Compiler::emitStorage()
{
	// Emit .data section for constants
	objectFile << endl;
	emit("SECTION", ".data");

	for (const auto& entry : symbolTable)
	{
		if (entry.second.getAlloc() == YES && entry.second.getMode() == CONSTANT)
			emit(entry.second.getInternalName(), "dd", entry.second.getValue(), "; " + entry.first);
	}

	// Emit .bss section for variables (and temporaries that had to be saved to memory)
	objectFile << endl;
	emit("SECTION", ".bss");

	for (const auto& entry : symbolTable)
	{
		if (entry.second.getAlloc() == YES && entry.second.getMode() == VARIABLE)
			emit(entry.second.getInternalName(), "resd", "1", "; " + entry.first.substr(0, 15));
	}
}

//***********************************************************************************************************

void Compiler::pushOperator(string name)
{
	operatorStk.push(name);
}

void Compiler::pushOperand(string name)
{
	// A literal gets its own constant in the symbol table the first time it is used
	if (isLiteral(name) && symbolTable.find(name) == symbolTable.end())
		insert(name, whichType(name), CONSTANT, whichValue(name), YES, 1);

	operandStk.push(name);
}

string Compiler::popOperator()
{
	if (operatorStk.empty())
		processError("compiler error; operator stack underflow");

	string topElement = operatorStk.top();
	operatorStk.pop();

	return topElement;
}

string Compiler::popOperand()
{
	if (operandStk.empty())
		processError("compiler error; operand stack underflow");

	string topElement = operandStk.top();
	operandStk.pop();

	return topElement;
}

//***********************************************************************************************************
// Helpers shared by the emit routines below.
//
// They are ordinary functions rather than members so that stage1.h can stay exactly as
// provided; the private state they need (symbol table, A register) is passed in by reference.
//
// contentsOfAReg (aReg) holds the name of whatever the A register (eax) currently holds.
// A temporary that lives only in eax is saved to memory ("spilled") before eax is reused.

static string mem(SymTab& st, const string& name)
{
	return "[" + st.at(name).getInternalName() + "]";
}

static void spillTemp(Compiler& c, SymTab& st, string& aReg, const string& keep1, const string& keep2)
{
	if (c.isTemporary(aReg) && aReg != keep1 && aReg != keep2)
	{
		st.at(aReg).setAlloc(YES);
		c.emit("", "mov", mem(st, aReg) + ",eax", "; save " + aReg + " before reusing the A register");
		aReg = "";
	}
}

static void loadA(Compiler& c, SymTab& st, string& aReg, const string& name)
{
	if (aReg != name)
	{
		c.emit("", "mov", "eax," + mem(st, name), "; load " + name + " into the A register");
		aReg = name;
	}
}

// For operations where the order of the operands matters, the left operand (op2) must be in eax
static void prepareLeft(Compiler& c, SymTab& st, string& aReg, const string& left)
{
	spillTemp(c, st, aReg, left, left);
	loadA(c, st, aReg, left);
}

// For operations where the order does not matter, use whichever operand is already in eax.
// Returns the operand that still has to be read from memory.
static string useA(Compiler& c, SymTab& st, string& aReg, const string& op1, const string& op2)
{
	spillTemp(c, st, aReg, op1, op2);

	if (aReg == op2)
		return op1;

	if (aReg == op1)
		return op2;

	loadA(c, st, aReg, op2);
	return op1;
}

static void releaseTemp(Compiler& c, string& aReg, const string& name)
{
	if (c.isTemporary(name))
	{
		if (aReg == name)
			aReg = "";

		c.freeTemp();
	}
}

// The result of an operation is left in eax and named by a fresh temporary
static void finishOperation(Compiler& c, SymTab& st, string& aReg, storeTypes resultType)
{
	string result = c.getTemp();

	st.at(result).setDataType(resultType);
	aReg = result;
	c.pushOperand(result);
}

static void finishBinary(Compiler& c, SymTab& st, string& aReg, const string& op1, const string& op2, storeTypes resultType)
{
	releaseTemp(c, aReg, op1);

	if (op2 != op1)
		releaseTemp(c, aReg, op2);

	finishOperation(c, st, aReg, resultType);
}

static void emitCommutative(Compiler& c, SymTab& st, string& aReg, const string& op1, const string& op2,
                            const string& instr, storeTypes resultType, const string& comment)
{
	string other = useA(c, st, aReg, op1, op2);

	c.emit("", instr, "eax," + mem(st, other), comment);
	finishBinary(c, st, aReg, op1, op2, resultType);
}

static void emitComparison(Compiler& c, SymTab& st, string& aReg, const string& op1, const string& op2,
                           const string& jump, bool orderMatters, const string& text)
{
	if (orderMatters)
	{
		prepareLeft(c, st, aReg, op2);
		c.emit("", "cmp", "eax," + mem(st, op1), "; compare " + op2 + " with " + op1);
	}

	else
	{
		string other = useA(c, st, aReg, op1, op2);
		c.emit("", "cmp", "eax," + mem(st, other), "; compare " + op2 + " with " + op1);
	}

	string trueLabel = c.getLabel();
	string endLabel = c.getLabel();

	c.emit("", jump, trueLabel, "; jump if " + text);
	c.emit("", "mov", "eax,0", "; false");
	c.emit("", "jmp", endLabel);
	c.emit(trueLabel + ":", "mov", "eax,-1", "; true");
	c.emit(endLabel + ":");

	finishBinary(c, st, aReg, op1, op2, BOOLEAN);
}

static void emitUnary(Compiler& c, SymTab& st, string& aReg, const string& op1,
                      const string& instr, storeTypes resultType, const string& comment)
{
	prepareLeft(c, st, aReg, op1);
	c.emit("", instr, "eax", comment);

	releaseTemp(c, aReg, op1);
	finishOperation(c, st, aReg, resultType);
}

//***********************************************************************************************************

void Compiler::emitReadCode(string operand, string operand2)
{
	string name;

	while (!operand.empty())
	{
		// Extract the next name from the operand string
		size_t pos = operand.find(',');

		if (pos != string::npos)
		{
			name = operand.substr(0, pos);
			operand.erase(0, pos + 1);
		}

		else
		{
			name = operand;
			operand.clear();
		}

		auto it = symbolTable.find(name);

		if (it == symbolTable.end())
			processError("reference to undefined symbol: " + name);

		if (it->second.getDataType() != INTEGER)
			processError("can't read variables of this type: " + name);

		if (it->second.getMode() != VARIABLE)
			processError("attempting to read to a read-only location: " + name);

		emit("", "call", "ReadInt", "; read int; value placed in eax");
		emit("", "mov", mem(symbolTable, name) + ",eax", "; store eax at " + name);

		contentsOfAReg = name;
	}
}

void Compiler::emitWriteCode(string operand, string operand2)
{
	string name;

	while (!operand.empty())
	{
		size_t pos = operand.find(',');

		if (pos != string::npos)
		{
			name = operand.substr(0, pos);
			operand.erase(0, pos + 1);
		}

		else
		{
			name = operand;
			operand.clear();
		}

		auto it = symbolTable.find(name);

		if (it == symbolTable.end())
			processError("reference to undefined symbol: " + name);

		if (it->second.getDataType() != INTEGER && it->second.getDataType() != BOOLEAN)
			processError("can't write variables of this type: " + name);

		loadA(*this, symbolTable, contentsOfAReg, name);

		emit("", "call", "WriteInt", "; write int in eax to standard out");
		emit("", "call", "Crlf", "; write \\r\\n to standard out");
	}
}

void Compiler::emitAssignCode(string operand1, string operand2)         // op2 = op1
{
	auto it1 = symbolTable.find(operand1);
	auto it2 = symbolTable.find(operand2);

	if (it1 == symbolTable.end())
		processError("reference to undefined symbol: " + operand1);

	if (it2 == symbolTable.end())
		processError("reference to undefined symbol: " + operand2);

	if (it1->second.getDataType() != it2->second.getDataType())
		processError("incompatible types for operator ':='");

	if (it2->second.getMode() != VARIABLE)
		processError("symbol on left-hand side of assignment must have a storage mode of VARIABLE");

	// x := x needs no code
	if (operand1 == operand2)
		return;

	loadA(*this, symbolTable, contentsOfAReg, operand1);

	emit("", "mov", mem(symbolTable, operand2) + ",eax", "; " + operand2 + " = AReg");
	contentsOfAReg = operand2;

	if (isTemporary(operand1))
		freeTemp();
}

void Compiler::emitAdditionCode(string operand1, string operand2)       // op2 +  op1
{
	if (whichType(operand1) != INTEGER || whichType(operand2) != INTEGER)
		processError("binary '+' requires integer operands");

	emitCommutative(*this, symbolTable, contentsOfAReg, operand1, operand2, "add", INTEGER,
	                "; AReg = " + operand2 + " + " + operand1);
}

void Compiler::emitSubtractionCode(string operand1, string operand2)    // op2 -  op1
{
	if (whichType(operand1) != INTEGER || whichType(operand2) != INTEGER)
		processError("binary '-' requires integer operands");

	prepareLeft(*this, symbolTable, contentsOfAReg, operand2);
	emit("", "sub", "eax," + mem(symbolTable, operand1), "; AReg = " + operand2 + " - " + operand1);

	finishBinary(*this, symbolTable, contentsOfAReg, operand1, operand2, INTEGER);
}

void Compiler::emitMultiplicationCode(string operand1, string operand2) // op2 *  op1
{
	if (whichType(operand1) != INTEGER || whichType(operand2) != INTEGER)
		processError("binary '*' requires integer operands");

	emitCommutative(*this, symbolTable, contentsOfAReg, operand1, operand2, "imul", INTEGER,
	                "; AReg = " + operand2 + " * " + operand1);
}

void Compiler::emitDivisionCode(string operand1, string operand2)       // op2 /  op1
{
	if (whichType(operand1) != INTEGER || whichType(operand2) != INTEGER)
		processError("operator 'div' requires integer operands");

	prepareLeft(*this, symbolTable, contentsOfAReg, operand2);
	emit("", "cdq", "", "; sign-extend eax into edx:eax");
	emit("", "idiv", "dword " + mem(symbolTable, operand1), "; AReg = " + operand2 + " div " + operand1);

	finishBinary(*this, symbolTable, contentsOfAReg, operand1, operand2, INTEGER);
}

void Compiler::emitModuloCode(string operand1, string operand2)         // op2 %  op1
{
	if (whichType(operand1) != INTEGER || whichType(operand2) != INTEGER)
		processError("operator 'mod' requires integer operands");

	prepareLeft(*this, symbolTable, contentsOfAReg, operand2);
	emit("", "cdq", "", "; sign-extend eax into edx:eax");
	emit("", "idiv", "dword " + mem(symbolTable, operand1), "; remainder goes to edx");
	emit("", "mov", "eax,edx", "; AReg = " + operand2 + " mod " + operand1);

	finishBinary(*this, symbolTable, contentsOfAReg, operand1, operand2, INTEGER);
}

void Compiler::emitNegationCode(string operand1, string operand2)       // -op1
{
	if (whichType(operand1) != INTEGER)
		processError("unary '-' requires an integer operand");

	emitUnary(*this, symbolTable, contentsOfAReg, operand1, "neg", INTEGER, "; AReg = -" + operand1);
}

void Compiler::emitNotCode(string operand1, string operand2)            // !op1
{
	if (whichType(operand1) != BOOLEAN)
		processError("operator 'not' requires a boolean operand");

	emitUnary(*this, symbolTable, contentsOfAReg, operand1, "not", BOOLEAN, "; AReg = not " + operand1);
}

void Compiler::emitAndCode(string operand1, string operand2)            // op2 && op1
{
	if (whichType(operand1) != BOOLEAN || whichType(operand2) != BOOLEAN)
		processError("operator 'and' requires boolean operands");

	emitCommutative(*this, symbolTable, contentsOfAReg, operand1, operand2, "and", BOOLEAN,
	                "; AReg = " + operand2 + " and " + operand1);
}

void Compiler::emitOrCode(string operand1, string operand2)             // op2 || op1
{
	if (whichType(operand1) != BOOLEAN || whichType(operand2) != BOOLEAN)
		processError("operator 'or' requires boolean operands");

	emitCommutative(*this, symbolTable, contentsOfAReg, operand1, operand2, "or", BOOLEAN,
	                "; AReg = " + operand2 + " or " + operand1);
}

void Compiler::emitEqualityCode(string operand1, string operand2)       // op2 == op1
{
	if (whichType(operand1) != whichType(operand2))
		processError("incompatible types for operator '='");

	emitComparison(*this, symbolTable, contentsOfAReg, operand1, operand2, "je", false,
	               operand2 + " = " + operand1);
}

void Compiler::emitInequalityCode(string operand1, string operand2)     // op2 != op1
{
	if (whichType(operand1) != whichType(operand2))
		processError("incompatible types for operator '<>'");

	emitComparison(*this, symbolTable, contentsOfAReg, operand1, operand2, "jne", false,
	               operand2 + " <> " + operand1);
}

void Compiler::emitLessThanCode(string operand1, string operand2)       // op2 <  op1
{
	if (whichType(operand1) != INTEGER || whichType(operand2) != INTEGER)
		processError("operator '<' requires integer operands");

	emitComparison(*this, symbolTable, contentsOfAReg, operand1, operand2, "jl", true,
	               operand2 + " < " + operand1);
}

void Compiler::emitLessThanOrEqualToCode(string operand1, string operand2) // op2 <= op1
{
	if (whichType(operand1) != INTEGER || whichType(operand2) != INTEGER)
		processError("operator '<=' requires integer operands");

	emitComparison(*this, symbolTable, contentsOfAReg, operand1, operand2, "jle", true,
	               operand2 + " <= " + operand1);
}

void Compiler::emitGreaterThanCode(string operand1, string operand2)    // op2 >  op1
{
	if (whichType(operand1) != INTEGER || whichType(operand2) != INTEGER)
		processError("operator '>' requires integer operands");

	emitComparison(*this, symbolTable, contentsOfAReg, operand1, operand2, "jg", true,
	               operand2 + " > " + operand1);
}

void Compiler::emitGreaterThanOrEqualToCode(string operand1, string operand2) // op2 >= op1
{
	if (whichType(operand1) != INTEGER || whichType(operand2) != INTEGER)
		processError("operator '>=' requires integer operands");

	emitComparison(*this, symbolTable, contentsOfAReg, operand1, operand2, "jge", true,
	               operand2 + " >= " + operand1);
}
