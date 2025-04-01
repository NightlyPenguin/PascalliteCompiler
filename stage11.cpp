// Luis Adan Arreola
// CS 4301
// Stage 0

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <ctime>
#include <cctype>
#include <stage1.h>
#include <algorithm>
#include <cstdio>
#include <sstream>
#include <iomanip>

using namespace std;

// Global Variable(s)
int labelCounter = 0;
bool hasReachedEnd = false;

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
	
	char firstChar = s[0];
	size_t length = s.length();
	
	if ((isdigit(firstChar)) || (firstChar == '-' && length > 1 && isdigit(s[1])) ||
		(firstChar == '+' && length > 1 && isdigit(s[1])))
	{
		for (size_t i = 1; i < length; ++i)
		{
			// Check if character is not a digit or if it is a decimal point
			if (!isdigit(s[i]) || s[i] == '.')
			{
				// If a decimal point is found, return false
				if (s[i] == '.')
					return false;
				
				else 
					return false;
			}
		}
		return true;
	}
	
	// If the string starts with a decimal point, it is not an integer
	if (firstChar == '.')
		return false;
	
	return false;
}

bool Compiler::isBoolean(string str) const
{
	if (str == "true" || str == "false")
		return true;

	return false;
}

bool Compiler::isNonKeyId(string str) const
{
	if (str.empty())
		return false;
	
	// Check if any character in the string is a special symbol
	for (char c : str) 
	{
		if (isSpecialSymbol(c))
			return false;
	}
	
	// Check if the string is an integer
	if (isInteger(str))
		return false; 
	
	// Check if string is a keyword
	if (isKeyword(str))
		return false;
	
	return true;
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
			internName =  "I" + to_string(integers);
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
            // Optionally handle any other unforeseen cases
            internName = "X" + to_string(unknowns);
            unknowns++;
            break;
	}
	
	return internName;
}

bool isOperand(string name) 
{
	if (name == "=" || name == "<>" || 
	    name == "<=" || name == ">=" || 
		name == "<" || name == ">") 
	{
			return true;
	}
	
	return false;
}

bool isAddLevelOperand(const string& s)
{
	return s == "+" || s == "-";
}

bool isMulLevelOp(const string& s)
{
    // Check if the string is a multiplication-level operator
    return s == "*" || s == "/" || s == "div" || s == "mod" || s == "and";
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

	listingFile << "STAGE0:  " << "   Luis Arreola & Htoo Myat       " <<  ctime(&now) << endl;
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
	
	//Call exit() to terminate program
	exit(EXIT_FAILURE);
}

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
	// Assume that token is a class member that holds the current token
	if (token != "var") 
		processError("keyword 'var' expected");
	
	else 
	{
		// Move to the next token after "var"
		token = nextToken();
		
		// Check if the next token is a non-keyword identifier
		if (!isNonKeyId(token)) 
			processError("non-keyword identifier must follow 'var'");
		
		else
			varStmts();
	}	
}


void Compiler::beginEndStmt() //token should be "begin"
{
	if(token != "begin")
		processError("keyword 'begin' expected");
	
	if(isNonKeyId(nextToken())||token == "read"||token == "write")
		execStmts();
	
	if(token != "end")
		processError("non-key id, 'read', 'write' or 'end' expected");
	
	if(nextToken() != ".")
		processError("period expected after keyword 'end'");
	
	code("end","","");
	nextToken();
}



void Compiler::constStmts() //token should be NON_KEY_ID 				  		isliteral
{ 
	string x,y;
	
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
		if(!isInteger(nextToken())) 
			processError("integer expected after sign");
			
		
		y = y + token;
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
				processError("expected BOOLEAN after 'not' statement");
				
			else 
					y = (whichValue(token) == "1") ? "false" : "true";
		}
	}

	if (isNonKeyId(y)) 
	{
		// Check if y is in the symbol table and if its type is not PROG_NAME.
		auto SymbolI = symbolTable.find(y);
		
		if (SymbolI != symbolTable.end() && SymbolI->second.getDataType() == PROG_NAME)
			processError("Data type of token on the right-hand side must be INTEGER or BOOLEAN");
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

	// Check if the current token is a non-keyword identifier
	if (!isNonKeyId(token)) 
		processError("non-keyword identifier expected");
	
	else 
	{
		// Retrieve a comma-separated list of variable names
		x = ids(); // ids() should update 'token' to the character after the identifier list
	
		// After ids(), 'token' should be the colon. If it's not, an error is generated.
		if (token != ":") 
			processError("\":\" expected"); 
		
		else 
		{
			// Get the next token, which should specify the type
			token = nextToken();
			
			if (token != "integer" && token != "boolean") 
				processError("illegal type follows \":\"");
			
			
			else 
			{
				y = token;
				token = nextToken();
				
				if (token != ";") 
					processError("semicolon expected after type specifier");
				
				else 
				{	
					// Insert the variables into the symbol table with their type
					if (y == "integer") 
						insert(x, INTEGER, VARIABLE, "", YES, 1);
					
					
					else if (y == "boolean") 
						insert(x, BOOLEAN, VARIABLE, "", YES, 1);
					
					// Move to the next token after the semicolon
					token = nextToken();
					
					// Expecting either another variable declaration (non-key id) or the 'begin' keyword
					if (token != "begin" && !isNonKeyId(token)) 
						processError("non-keyword identifier or 'begin' expected after variable declaration");
				
					
					else if (isNonKeyId(token))
						varStmts();
				}
			}
		}
	}
}




string Compiler::ids() //token should be NON_KEY_ID 				  
{ 
	string temp,tempString;
	
	if (!isNonKeyId(token))
		processError("non-keyword identifier expected");
	
	tempString = token;
	temp = token;
	
	if(nextToken() == ",") 
	{
		if (!isNonKeyId(nextToken())) 
			processError("non-keyword identifier expected");
		
		tempString = temp + "," + ids();
	}
	
	return tempString;
}




void Compiler::insert(string externalName, storeTypes inType, modes inMode, string inValue, allocation inAlloc, int inUnits)
{
    // Split the externalName string by commas to handle multiple names
    stringstream ss(externalName);
    string name;
    while (getline(ss, name, ',')) 
	{
        name.erase(remove_if(name.begin(), name.end(), ::isspace), name.end());

        // Check for multiple definitions or illegal use of keywords
        if (symbolTable.find(name) != symbolTable.end()) {
            processError("Multiple name definition for: " + name);
        } else if (isKeyword(name)) {
            processError("Illegal use of keyword: " + name);
        } else {
            // Generate internal name based on the type of identifier
            string internalName = isupper(name[0]) ? name : genInternalName(inType);
            
            // Insert the new symbol into the symbol table
            //symbolTable[name] = SymbolTableEntry(internalName, inType, inMode, inValue, inAlloc, inUnits);
			symbolTable.emplace(std::make_pair(name, SymbolTableEntry(internalName, inType, inMode, inValue, inAlloc, inUnits)));

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
		
		else
		{
			processError("Reference to undefined constant");
			return PROG_NAME;
		}
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
	if (symbolTable.count(name) > 0)
	{
		SymbolTableEntry entry = symbolTable.at(name);
		
		if (entry.getMode() == CONSTANT)
			return entry.getValue();
	}
	

	
	processError("Reference to undefined constant or variable: " + name);
	return ""; // Returns an empty string as default/error
}



void Compiler::code(string op, string operand1, string operand2)
{
	if (op == "program") 
		emitPrologue(operand1); 
	
	else if(op == "end")
		emitEpilogue();
	
	else if (op == "read")  		// emit read code
		emitReadCode(operand1);
	
	else if (op == "write")			//emit write code
		emitWriteCode(operand1);
	
	else if (op == "+") 			// this must be binary '+'			emit addition code
		emitAdditionCode(operand1, operand2);
	
	else if (op == "-") 			// this must be binary '-' 			emit subtraction code
		emitSubtractionCode(operand1, operand2);
	
	else if (op == "neg")		 	// this must be unary '-'			emit negation code;
		emitNegationCode(operand1);
	
	else if (op == "not")			// emit not code
		 emitNotCode(operand1);
	
	else if (op == "*")				// emit multiplication code
		emitMultiplicationCode(operand1, operand2);
	
	else if (op == "div")			// emit division code
		emitDivisionCode(operand1, operand2);
	
	else if (op == "mod")			// emit modulo code
		emitModuloCode(operand1, operand2);
		
	else if (op == "and")			// emit and code
		emitAndCode(operand1, operand2);
	
	else if (op == "=")				// emit equality code
		emitEqualityCode(operand1, operand2);
	
	else if (op == ":=")			// emit assignment code
		emitAssignCode(operand1, operand2);
		
	else 
		processError("compiler error since function code should not be called with illegal arguments");
	
}




string Compiler::getLabel()
{
	labelCounter++;
	
	string label = "L" + std::to_string(labelCounter);
	
	return label;
}


void Compiler::freeTemp()
{
	currentTempNo--;
	
	if (currentTempNo < -1)
		processError("compiler error, currentTempNo should be ≥ –1");
}

string Compiler::getTemp()
{
	string temp;
	currentTempNo++;
	
	//  temp = "T" + currentTempNo;
	temp = "T" + std::to_string(currentTempNo);
	
	if (currentTempNo > maxTempNo)
	{
		insert(temp, UNKNOWN, VARIABLE, "", NO, 1);
		maxTempNo++;
	}
	
	return temp;
}



bool Compiler::isTemporary(string s) const
{
    // Check if the string starts with 'T' followed by digit(s)
    if (!s.empty() && s[0] == 'T')
    {
        for (size_t i = 1; i < s.size(); ++i)
        {
            if (!isdigit(s[i]))
            {
                return false; 
            }
        }
        return true; 
    }
    return false;    
}



void Compiler::execStmts()      // stage 1, production 2
{
	// As long as the current token is the beginning of an executable statement, process it
	while (isNonKeyId(token) || token == "read" || token == "write" || token == "if" || token == "while" || token == "repeat" || token == ";" || token == "begin")
	{
		execStmt(); 
		
		// Move to the next token if the current one is a semicolon
		if (token == ";")
			nextToken();
	}	
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
		processError("Executable statement type not recognized");		
}

void Compiler::assignStmt()     // stage 1, production 4
{
	// Check if the current token is a non-keyword identifier (the left-hand side of the assignment)
	if (!isNonKeyId(token)) 
		processError("non-keyword identifier expected for assignment");
	
	pushOperand(token);
	
	if (nextToken() != ":=") 
		processError("':=' expected for assignment");
	
	pushOperator(":=");
	express();
	
	// After processing the expression, the next token should be ';'
	if (nextToken() != ";") 
		processError("';' expected at the end of assignment statement");
	
	string rhs = popOperand(); 
	string lhs = popOperand(); 
	string op = popOperator(); 
	
	code(op, rhs, lhs);
}



void Compiler::readStmt()       // stage 1, production 5
{
	if (nextToken() != "(")
	{
		processError("'(' expected after 'read'");
		return;
	}
	
	nextToken();
	string idList = ids();
	
	// Iterate through the idList and generate read code for each identifier
	size_t start = 0;
	size_t end = idList.find(",");
	
	while (end != string::npos)
	{
		string id = idList.substr(start, end - start);
		code("read", id);
	
		start = end + 1;
		end = idList.find(",", start);
	}
	
	// Process the last identifier (or the only one if there was no comma)
	code("read", idList.substr(start));
	
	// Check and consume ')'
	if (token != ")")
	{
		processError("')' expected after identifier list in 'read' statement");
		return;
	}
	
	// Move to the next token and check for ';'
	if (nextToken() != ";")
	{
		processError("';' expected after 'read' statement");
	}
}




void Compiler::writeStmt()      // stage 1, production 7
{
	// Expecting '(' after 'read'
	if (nextToken() != "(")
	{
		processError("'(' expected after 'read'");
		return;
	}
	
	// Fetch the next token, which should start the list of identifiers
	nextToken();
	
	// Collect and process identifiers, separated by commas
	string idList = ids();
	string id = "";
	for (size_t i = 0; i < idList.length(); i++)
	{
		if (idList[i] == ',')
		{
			if (!id.empty())
			{
				code("read", id);
				id = "";
			}
		}
		else
		{
			id += idList[i];
		}
	}
	
	// Process the last identifier in the list
	if (!id.empty())
	{
		code("read", id);
	}
	
	// Check and consume ')'
	if (token != ")")
	{
		processError("')' expected after identifier list in 'read' statement");
		return;
	}
	
	// Move to the next token and check for ';'
	if (nextToken() != ";")
	{
		processError("';' expected after 'read' statement");
	}
}



void Compiler::express()        // stage 1, production 9
{
	nextToken();
	
	if (!(token == "not" || isBoolean(token) || token == "(" || token == "+" || token == "-" || isInteger(token) || isNonKeyId(token)))
	{
		processError("'not', '(', '+', '-', boolean literal, integer literal, or non-keyword identifier expected");
		return;
	}
	
	term();
	expresses();
}


void Compiler::expresses()      // stage 1, production 10
{
	// Check if the current token is a relational operator
	if (isOperand(token)) 
	{
		pushOperator(token);  // Push the operator onto the operator stack
		token = nextToken();  // Get the next token
		term();               // Process the term
	
		// Pop operands and operator to generate code
		string rhs = popOperand();
		string lhs = popOperand();
		code(popOperator(), lhs, rhs);
	
		// Recursively call expresses to handle further expressions
		expresses();
	}

}

void Compiler::term()           // stage 1, production 11
{
	// Check if the token is a valid starting symbol for a TERM
	if (!(token == "not" || isBoolean(token) || token == "(" || token == "+" || token == "-" || isInteger(token) || isNonKeyId(token)))
	{
		processError("'not', '(', '+', '-', boolean literal, integer literal, or non-keyword identifier expected in term");
		return;
	}
	
	factor();
	terms();
}


void Compiler::terms()          // stage 1, production 12
{
/*	while (isAddLevelOperand(token)) 
	{
		// Push the current operator (like '+' or '-') onto the operator stack
		pushOperator(token);
	
		// Move to the next token and process the next factor
		token = nextToken();
		factor();
	
		// After processing the factor, pop the operands and the operator
		string rhs = popOperand(); // Right-hand side operand
		string lhs = popOperand(); // Left-hand side operand
		string op = popOperator(); // Operator
	
		// Generate code for the operation
		code(op, lhs, rhs);
	}*/
	
	
	
    while (token == "*" || token == "div" || token == "mod")
    {
        string op = token;  // Store the operator
        nextToken();  // Consume the operator

        // Process the next factor in the term
        factor();

        // Pop the two operands from the operand stack
        string operand2 = popOperand();
        string operand1 = popOperand();

        // Generate code based on the operator
        if (op == "*")
            emitMultiplicationCode(operand1, operand2);
        else if (op == "div")
            emitDivisionCode(operand1, operand2);
        else if (op == "mod")
            emitModuloCode(operand1, operand2);

        // Push the result onto the operand stack
        string resultTemp = getTemp();
        insert(resultTemp, whichType(operand1), VARIABLE, "", YES, 1);
        pushOperand(resultTemp);
    }
}


void Compiler::factor()         // stage 1, production 13
{
	// Check if the token is a valid starting symbol for a FACTOR
    if (!(token == "not" || isBoolean(token) || token == "(" || token == "+" || token == "-" || isInteger(token) || isNonKeyId(token)))
    {
        processError("'not', '(', '+', '-', boolean literal, integer literal, or non-keyword identifier expected in factor");
    }

    part();
    factors();
}

/*
void Compiler::factors()        // stage 1, production 14
{
	// Check for and handle multiplication-level operators
	if (isMulLevelOp(token)) 
	{
		pushOperator(token);   
		nextToken();   			
		part();                
	
		// Pop the operands and operator, then generate code
		string rhs = popOperand();    
		string lhs = popOperand();    
		string op = popOperator();    
	
		code(op, lhs, rhs);           
		factors();
	}
}*/


void Compiler::factors()
{
    // Check if there are more factors following the current factor
    while (token == "*" || token == "div" || token == "mod" || token == "and")
    {
        string op = token;
        nextToken(); // Consume the operator

        // Process the next part in the expression
        part();

        // Pop the two operands from the operand stack
        string operand2 = popOperand();
        string operand1 = popOperand();

        // Now generate code based on the operator
        if (op == "*")
            emitMultiplicationCode(operand1, operand2);
        else if (op == "div")
            emitDivisionCode(operand1, operand2);
        else if (op == "mod")
            emitModuloCode(operand1, operand2);
        else if (op == "and")
            emitAndCode(operand1, operand2);

        // Push the result onto the operand stack
        string resultTemp = getTemp();
        insert(resultTemp, whichType(operand1), VARIABLE, "", YES, 1);
        pushOperand(resultTemp);
    }
}



/*
void Compiler::part()           // stage 1, production 15
{
	if (token == "not")
	{
		nextToken();
		
		if (token == "(")
		{
			express();
			
			if (token != ")")
				processError("')' expected");
			
			nextToken();
			string rhs = popOperand();
			code("not", rhs);
		}
		else if (isBoolean(token))
		{
			pushOperand(whichValue(token) == "1" ? "false" : "true");
			nextToken();
		}
		else if (isNonKeyId(token))
		{
			code("not", genInternalName(BOOLEAN));
			nextToken();
		}
		else
			processError("'(', boolean literal, or non-keyword identifier expected");
	}
	
	else if (token == "+")
	{
		nextToken();
		
		if (token == "(")
		{
			express();
			
			if (token != ")")
				processError("')' expected");
			
			nextToken();
		}
		
		else if (isInteger(token) || isNonKeyId(token))
		{
			pushOperand(token);
			nextToken();
		}
		
		else
			processError("'(', integer literal, or non-keyword identifier expected");
	}
	
	else if (token == "-")
	{
		nextToken();
		
		if (token == "(")
		{
			express();
			
			if (token != ")")
				processError("')' expected");
			
			nextToken();
			string rhs = popOperand();
			code("neg", rhs);
		}
		
		else if (isInteger(token))
		{
			pushOperand("-" + token);
			nextToken();
		}
		
		else if (isNonKeyId(token))
		{
			code("neg", genInternalName(INTEGER));
			nextToken();
		}
		
		else
			processError("'(', integer literal, or non-keyword identifier expected");
	}
	
	else if (isInteger(token) || isBoolean(token) || isNonKeyId(token))
	{
		pushOperand(token);
		nextToken();
	}
	
	else if (token == "(")
	{
		express();
		
		if (token != ")")
			processError("')' expected");
		nextToken();
	}
	
	else
		processError("'not', '(', '+', '-', boolean literal, integer literal, or non-keyword identifier expected");
}
*/


void Compiler::part()           // stage 1, production 15
{
    string operand;

    // Handle 'not' unary operator
    if (token == "not")
    {
        nextToken(); // Consume 'not'
        part(); // Process the operand of 'not'
        operand = popOperand(); // Get the operand from the stack

        if (whichType(operand) != BOOLEAN)
            processError("Operand of 'not' must be boolean");

        code("not", operand); // Generate code for 'not'
    }
    else if (token == "+")
    {
        // Handle unary '+'
        nextToken(); // Consume '+'
        part(); // Process the operand
        // Unary '+' does not change the value, so no code generation is required
    }
    else if (token == "-")
    {
        // Handle unary '-'
        nextToken(); // Consume '-'
        part(); // Process the operand
        operand = popOperand(); // Get the operand from the stack

        if (whichType(operand) != INTEGER)
            processError("Operand of unary '-' must be integer");

        code("neg", operand); // Generate code for negation
    }
    else if (token == "(")
    {
        // Handle expressions enclosed in parentheses
        nextToken(); // Consume '('
        express(); // Process the expression inside parentheses
        if (token != ")")
            processError("Missing ')'");

        nextToken(); // Consume ')'
    }
    else if (isInteger(token) || isBoolean(token) || isNonKeyId(token))
    {
        // Handle integer, boolean literals, and non-key identifiers
        if (isNonKeyId(token))
        {
            if (symbolTable.find(token) == symbolTable.end())
                processError("Reference to undefined identifier");

            // Check if the identifier is a constant or variable
            SymbolTableEntry entry = symbolTable.at(token);
            if (entry.getMode() == CONSTANT)
                operand = entry.getValue();
            else
                operand = token; // Use the identifier itself as the operand
        }
        else
        {
            // For literals, use the token itself as the operand
            operand = token;
        }

        pushOperand(operand); // Push the operand onto the operand stack
        nextToken(); // Consume the token
    }
    else
    {
        processError("Invalid expression part");
    }
}













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
					// Process error: unexpected end of file
					processError("unexpected end of file");
					return token;
				}
			}
			nextChar(); // Move past the closing '}'
		}
		else if (ch == '}')
		{
			// Process error: '}' cannot begin a token
			processError("'}' cannot begin a token");
			nextChar(); // Move past the current '}' character
		}
		else if (isspace(ch))
			nextChar();
			
		else if (isdigit(ch))
		{
			token += ch;
			
			while (isdigit(nextChar()) || ch == '.')
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
		
		else if (islower(ch))
		{
			token += ch;
			
			while (islower(nextChar()) || isdigit(ch) || ch == '_')
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
		
		else if (isdigit(ch))
		{
			token += ch;
			
			while (isdigit(nextChar()))
				token += ch;
			
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
		{
			listingFile << setw(5) << ++lineNo << '|';
		}
		
		listingFile << ch;
	}
	
	oldChar = ch;
	return ch;
}






/* ******************************************************************************************************************************************** */

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
	emit("","Exit", "{0}");
	emitStorage();
}



void Compiler::emitStorage()
{
    // Emit .data section for constants
	objectFile << endl;
    emit("SECTION", ".data");

	// Iterate over symbolTable entries with allocation YES and storage mode CONSTANT
	for (const auto& entry : symbolTable) 
	{
		if (entry.second.getAlloc() == YES && entry.second.getMode() == CONSTANT) 
		{
			string asmDataType = "dd";
			string comment = "; " + entry.first;
			
			if (entry.first == "yes")
				emit(entry.second.getInternalName(), asmDataType, "-1", comment);
			else
				emit(entry.second.getInternalName(), asmDataType, entry.second.getValue(), comment);
		}
	}
	
	// Emit .bss section for variables
	objectFile << endl;
    emit("SECTION", ".bss");

    // Iterate over symbolTable entries with allocation YES and storage mode VARIABLE
    for (const auto& entry : symbolTable) 
	{
        if (entry.second.getAlloc() == YES && entry.second.getMode() == VARIABLE) 
		{
            string asmDataType = "resd";
			string comment = "; " + entry.first.substr(0, 15);
			
            emit(entry.second.getInternalName(), asmDataType, "1", comment);
        }
    }	
}


//***********************************************************************************************************

void Compiler::pushOperator(string name) 
{
	operatorStk.push(name);
}

void Compiler::pushOperand(string name)
{
	if (isLiteral(name) && symbolTable.find(name) == symbolTable.end())
	{
		storeTypes type = whichType(name);
		insert(name, type, CONSTANT, name, NO, 1);
	}
	
	operandStk.push(name);
}


string Compiler::popOperator()
{
	if (!operatorStk.empty())
	{
		string topElement = operatorStk.top();
		operatorStk.pop();
		
		return topElement;
	}
	else
	{
		processError("compiler error; operator stack underflow");
		return "";
	}
}

string Compiler::popOperand()
{
	if (!operandStk.empty())
	{
		string topElement = operandStk.top();
		operandStk.pop();
		
		return topElement;
	}
	else
	{
		processError("compiler error; operand stack underflow");
		return ""; 
	}
}



//***********************************************************************************************************
void Compiler::emitReadCode(string operand, string operand2)
{
	string name;
	
	while (!operand.empty())
	{
		// Extract the next name from the operand string
		size_t pos = operand.find_first_of(',');
		
		if (pos != string::npos)
		{
			name = operand.substr(0, pos);
			operand.erase(0, pos + 1); // Remove the extracted name and the comma
		}
		
		else
		{
			name = operand; // Last name in the operand string
			operand.clear();
		}
	
		// Check if the name is in the symbol table
		auto it = symbolTable.find(name);
		if (it == symbolTable.end())
		{
			processError("Reference to undefined symbol: " + name);
			continue; // Skip processing this name
		}
	
		// Check if the data type of the name is INTEGER
		if (it->second.getDataType() != INTEGER)
		{
			processError("Can't read variables of this type: " + name);
			continue; // Skip processing this name
		}
	
		// Check if the storage mode of the name is VARIABLE
		if (it->second.getMode() != VARIABLE)
		{
			processError("Attempting to read to a read-only location: " + name);
			continue; // Skip processing this name
		}
	
		emit("call", "ReadInt");
		emit("mov", "[" + it->second.getInternalName() + "]", "eax", "; Read integer into " + name);
	
		contentsOfAReg = it->second.getInternalName();
	}
}     






void Compiler::emitWriteCode(string operand, string operand2)
{
	string name;
	size_t pos = 0;
	
	while ((pos = operand.find(',')) != string::npos || !operand.empty())
	{
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
		{
			processError("reference to undefined symbol: " + name);
			continue;
		}
	
		if (contentsOfAReg != name)
		{
			emit("mov", "eax", "[" + it->second.getInternalName() + "]");
			contentsOfAReg = name;
		}
	
		if (it->second.getDataType() == INTEGER || it->second.getDataType() == BOOLEAN)
		{
			emit("call", "WriteInt");
			emit("call", "Crlf");
		}
	}
}


void Compiler::emitAssignCode(string operand1, string operand2)
{
	// Check existence of operand1 and operand2 in the symbol table
	auto it1 = symbolTable.find(operand1);
	auto it2 = symbolTable.find(operand2);
	
	if (it1 == symbolTable.end() || it2 == symbolTable.end())
	{
		processError("reference to undefined symbol: " + (it1 == symbolTable.end() ? operand1 : operand2));
		return;
	}
	
	// Check if the types of operands are not the same
	if (it1->second.getDataType() != it2->second.getDataType())
	{
		processError("incompatible types");
		return;
	}
	
	// Check if storage mode of operand2 is not VARIABLE
	if (it2->second.getMode() != VARIABLE)
	{
		processError("symbol on left-hand side of assignment must have a storage mode of VARIABLE");
		return;
	}
	
	// If operand1 is the same as operand2, no action is necessary
	if (operand1 == operand2) return;
	
	// If operand1 is not in the A register then
	if (contentsOfAReg != operand1)
	{
		emit("mov", "eax", "[" + it1->second.getInternalName() + "]", "; Load " + operand1 + " into A register");
		contentsOfAReg = operand1;
	}
	
	emit("mov", "[" + it2->second.getInternalName() + "]", "eax", "; Store A register into " + operand2);
	contentsOfAReg = operand2;
	
	// If operand1 is a temp then free its name for reuse
	if (isTemporary(operand1))
		freeTemp();
}





void Compiler::emitAdditionCode(string operand1, string operand2)	// op2 +  op1
{
	// Check existence of operand1 and operand2 in the symbol table
	auto it1 = symbolTable.find(operand1);
	auto it2 = symbolTable.find(operand2);
	
	if (it1 == symbolTable.end() || it2 == symbolTable.end())
	{
		processError("reference to undefined symbol: " + (it1 == symbolTable.end() ? operand1 : operand2));
		return;
	}
	
	// Check if the type of either operand is not integer
	if (it1->second.getDataType() != INTEGER || it2->second.getDataType() != INTEGER)
	{
		processError("illegal type for addition operation");
		return;
	}
	
	// If neither operand is in the A register then
	if (contentsOfAReg != operand1 && contentsOfAReg != operand2)
	{
		// Emit code to load operand2 into the A register
		emit("mov", "eax", "[" + it2->second.getInternalName() + "]", "; Load " + operand2 + " into A register");
		contentsOfAReg = operand2;
	}
	
	// Emit code to perform register-memory addition with operand1
	emit("add", "eax", "[" + it1->second.getInternalName() + "]", "; Add " + operand1 + " to A register");
	
	// The result is now in the A register. If operand1 is a temp, free it for reuse
	if (isTemporary(operand1))
		freeTemp();
	
	string tempResult = getTemp();
	// insert(tempResult, INTEGER, VARIABLE, "", YES, 1);
	symbolTable.insert({tempResult, SymbolTableEntry(tempResult, INTEGER, VARIABLE, "", YES, 1)});
	pushOperand(tempResult);
	
	contentsOfAReg = tempResult; // Update contents of A register to the new temp
}




void Compiler::emitMultiplicationCode(string operand1, string operand2)	// op2 *  op1
{
	// Check existence of operand1 and operand2 in the symbol table
	auto it1 = symbolTable.find(operand1);
	auto it2 = symbolTable.find(operand2);
	
	if (it1 == symbolTable.end() || it2 == symbolTable.end())
	{
		processError("reference to undefined symbol: " + (it1 == symbolTable.end() ? operand1 : operand2));
		return;
	}
	
	// Check if the type of either operand is not integer
	if (it1->second.getDataType() != INTEGER || it2->second.getDataType() != INTEGER)
	{
		processError("illegal type for multiplication operation");
		return;
	}
	
	// If neither operand is in the A register then
	if (contentsOfAReg != operand1 && contentsOfAReg != operand2)
	{
		// Emit code to load operand2 into the A register
		emit("mov", "eax", "[" + it2->second.getInternalName() + "]", "; Load " + operand2 + " into A register");
		contentsOfAReg = operand2;
	}
	
	// Emit code to perform register-memory multiplication with operand1
	emit("imul", "eax", "[" + it1->second.getInternalName() + "]", "; Multiply " + operand1 + " with A register");
	
	// The result is now in the A register. If operand1 is a temp, free it for reuse
	if (isTemporary(operand1))
		freeTemp();
	

	string tempResult = getTemp();
	//insert(tempResult, INTEGER, VARIABLE, "", YES, 1);
	symbolTable.insert({tempResult, SymbolTableEntry(tempResult, INTEGER, VARIABLE, "", YES, 1)});
	pushOperand(tempResult);
	
	contentsOfAReg = tempResult; // Update contents of A register to the new temp
	
}


void Compiler::emitDivisionCode(string operand1, string operand2)	// op2 /  op1
{
	// Check existence of operand1 and operand2 in the symbol table
	auto it1 = symbolTable.find(operand1);
	auto it2 = symbolTable.find(operand2);
	
	if (it1 == symbolTable.end() || it2 == symbolTable.end())
	{
		processError("reference to undefined symbol: " + (it1 == symbolTable.end() ? operand1 : operand2));
		return;
	}
	
	// Check if the type of either operand is not integer
	if (it1->second.getDataType() != INTEGER || it2->second.getDataType() != INTEGER)
	{
		processError("illegal type for division operation");
		return;
	}
	
	// Ensure operand2 is in the A register because idiv divides the value in edx:eax by the operand
	if (contentsOfAReg != operand2)
	{
		// Emit code to move operand2 into the A register (eax)
		emit("mov", "eax", "[" + it2->second.getInternalName() + "]", "; Load " + operand2 + " into A register");
		contentsOfAReg = operand2;
	}
	
	emit("cdq", "", "", "; Sign-extend eax into edx");
	emit("idiv", "[" + it1->second.getInternalName() + "]", "", "; Divide edx:eax by " + operand1);
	
	// If operand1 is a temp, free it for reuse
	if (isTemporary(operand1))
		freeTemp();
	
	// Assign a new temporary name for the result (the quotient) and push it onto the operand stack
	string tempResult = getTemp();
	//insert(tempResult, INTEGER, VARIABLE, "", YES, 1);
	symbolTable.insert({tempResult, SymbolTableEntry(tempResult, INTEGER, VARIABLE, "", YES, 1)});
	pushOperand(tempResult);
	
	// Update contents of A register to the new temp (which now holds the quotient)
	contentsOfAReg = tempResult;
}



void Compiler::emitSubtractionCode(string operand1, string operand2)	// op2 -  op1
{
	// Check existence of operand1 and operand2 in the symbol table
	auto it1 = symbolTable.find(operand1);
	auto it2 = symbolTable.find(operand2);
	
	if (it1 == symbolTable.end() || it2 == symbolTable.end())
	{
		processError("reference to undefined symbol: " + (it1 == symbolTable.end() ? operand1 : operand2));
		return;
	}
	
	// Check if the type of either operand is not integer
	if (it1->second.getDataType() != INTEGER || it2->second.getDataType() != INTEGER)
	{
		processError("illegal type for subtraction operation");
		return;
	}
	
	// Ensure operand2 is in the A register because sub instruction will subtract operand1 from it
	if (contentsOfAReg != operand2)
	{
		// Emit code to move operand2 into the A register (eax)
		emit("mov", "eax", "[" + it2->second.getInternalName() + "]", "; Load " + operand2 + " into A register");
		contentsOfAReg = operand2;
	}
	
	// Emit code to perform the subtraction. sub subtracts operand1 from the A register (eax)
	emit("sub", "eax", "[" + it1->second.getInternalName() + "]", "; Subtract " + operand1 + " from A register");
	
	// The result is now in the A register. If operand1 is a temp, free it for reuse
	if (isTemporary(operand1))
		freeTemp();
	
	// Assign a new temporary name for the result and push it onto the operand stack
	string tempResult = getTemp();
	// insert(tempResult, INTEGER, VARIABLE, "", YES, 1); // Assume insert() enters a new temp into the symbol table
	symbolTable.insert({tempResult, SymbolTableEntry(tempResult, INTEGER, VARIABLE, "", YES, 1)});
	pushOperand(tempResult);
	
	// Update contents of A register to the new temp (which now holds the result of the subtraction)
	contentsOfAReg = tempResult;
}


void Compiler::emitAndCode(string operand1, string operand2)            // op2 && op1
{
	// Check existence of operand1 and operand2 in the symbol table
	auto it1 = symbolTable.find(operand1);
	auto it2 = symbolTable.find(operand2);
	
	if (it1 == symbolTable.end() || it2 == symbolTable.end())
	{
		processError("reference to undefined symbol: " + (it1 == symbolTable.end() ? operand1 : operand2));
		return;
	}
	
	// Check if the types of operands are not the same or if they are not integers
	if (it1->second.getDataType() != INTEGER || it2->second.getDataType() != INTEGER)
	{
		processError("subtraction requires operands of integer type");
		return;
	}
	
	// Emit code to load operand2 into the A register if it is not already there
	if (contentsOfAReg != operand2)
	{
		emit("mov", "eax", "[" + it2->second.getInternalName() + "]", "; Load " + operand2 + " into A register");
		contentsOfAReg = operand2;
	}
	
	// Emit code to subtract operand1 from operand2
	emit("sub", "eax", "[" + it1->second.getInternalName() + "]", "; Subtract " + operand1 + " from " + operand2);
	
	// Assign new temporary name for the result and push it onto stack
	string tempResult = getTemp(); 
	//insert(tempResult, INTEGER, VARIABLE, "", YES, 1); 
	symbolTable.insert({tempResult, SymbolTableEntry(tempResult, INTEGER, VARIABLE, "", YES, 1)});
	pushOperand(tempResult); 
	
	contentsOfAReg = tempResult; 
	
	// If operand1 or operand2 was a temporary, free it
	if (isTemporary(operand1)) freeTemp();
	if (isTemporary(operand2) && operand1 != operand2) freeTemp(); // Free operand2 if it is different from operand1
}


void Compiler::emitEqualityCode(string operand1, string operand2)       // op2 == op1
{
    // Check if operands exist and have the same type
    auto it1 = symbolTable.find(operand1);
    auto it2 = symbolTable.find(operand2);
    
	if (it1 == symbolTable.end() || it2 == symbolTable.end())
    {
        processError("reference to undefined symbol in equality check");
        return;
    }
    
	if (it1->second.getDataType() != it2->second.getDataType())
    {
        processError("incompatible types for equality check");
        return;
    }

    // Prepare operands in registers
    // Load operand2 into the A register
    emit("mov", "eax", "[" + it2->second.getInternalName() + "]", "; Load " + operand2 + " into A register for equality comparison");
    contentsOfAReg = operand2;

    // Compare operand1 with the value in the A register
    emit("cmp", "eax", "[" + it1->second.getInternalName() + "]", "; Compare " + operand1 + " with " + operand2);
    
    // Allocate labels for true and false branches
    string labelTrue = getLabel();
    string labelFalse = getLabel();
    string labelEnd = getLabel();

    // Jump to labelTrue if equal, otherwise continue to labelFalse
    emit("je", labelTrue, "", "; Jump to " + labelTrue + " if " + operand1 + " equals " + operand2);

    // False branch: load 0 (false) into A register
    emit(labelFalse + ":", "mov eax", "0", "; Load 0 (false) into A register");
    emit("jmp", labelEnd, "", "; Jump to the end of equality check");

    // True branch: load -1 (true) into A register
    emit(labelTrue + ":", "mov eax", "-1", "; Load -1 (true) into A register");

    // End label
    emit(labelEnd + ":", "", "", "; End of equality check");

    // Deassign temporaries if necessary
    if (isTemporary(operand1)) freeTemp();
    if (isTemporary(operand2) && operand1 != operand2) freeTemp();

    // Assign result to a temporary variable
    string resultTemp = getTemp();
    // insert(resultTemp, BOOLEAN, VARIABLE, "", YES, 1);
    symbolTable.insert({resultTemp, SymbolTableEntry(resultTemp, BOOLEAN, VARIABLE, "", YES, 1)});
	contentsOfAReg = resultTemp;

    // Push the result onto the operand stack
    pushOperand(resultTemp);
}


void Compiler::emitModuloCode(string operand1, string operand2)         // op2 %  op1
{
	// Check if operands exist and have the same integer type
	auto it1 = symbolTable.find(operand1);
	auto it2 = symbolTable.find(operand2);

	if (it1 == symbolTable.end() || it2 == symbolTable.end())
	{
		processError("reference to undefined symbol in modulo operation");
		return;
	}
	
	if (it1->second.getDataType() != INTEGER || it2->second.getDataType() != INTEGER)
	{
		processError("modulo operation requires integer operands");
		return;
	}
	
	// Prepare for division to get the remainder
	// Move operand2 into eax, and sign-extend to edx
	emit("mov", "eax", "[" + it2->second.getInternalName() + "]", "; Load " + operand2 + " into eax for modulo operation");
	emit("cdq", "", "", "; Sign-extend eax into edx for division");
	contentsOfAReg = operand2;
	
	// Divide edx:eax by operand1, remainder will be in edx
	emit("idiv", "[" + it1->second.getInternalName() + "]", "", "; Divide edx:eax by " + operand1 + " for modulo operation");
	
	// Move remainder from edx to a new temporary
	string tempResult = getTemp();
	//insert(tempResult, INTEGER, VARIABLE, "", YES, 1);
	symbolTable.insert({tempResult, SymbolTableEntry(tempResult, INTEGER, VARIABLE, "", YES, 1)});
	emit("mov", "[" + tempResult + "]", "edx", "; Move remainder into " + tempResult);
	
	// Update contentsOfAReg and operand stack
	contentsOfAReg = tempResult;
	pushOperand(tempResult);
	
	// Free temporary variables if necessary
	if (isTemporary(operand1)) freeTemp();
	if (isTemporary(operand2) && operand1 != operand2) freeTemp();

}

void Compiler::emitNegationCode(string operand1, string operand2)           // -op1
{
	// Check if operand exists in the symbol table and is of integer type
	auto it = symbolTable.find(operand1);
	
	if (it == symbolTable.end()) 
	{
		processError("reference to undefined symbol in negation operation: " + operand1);
		return;
	}
	
	if (it->second.getDataType() != INTEGER) 
	{
		processError("negation operation requires an integer operand");
		return;
	}
	
	// Load operand1 into the A register if not already there
	if (contentsOfAReg != operand1)
	{
		emit("mov", "eax", "[" + it->second.getInternalName() + "]", "; Load " + operand1 + " into A register for negation");
		contentsOfAReg = operand1;
	}
	
	// Perform negation
	emit("neg", "eax", "", "; Negate the value in A register");
	
	// Assign result to a new temporary variable and update the symbol table
	string tempResult = getTemp();
	
	// insert(tempResult, INTEGER, VARIABLE, "", YES, 1);
	symbolTable.insert({tempResult, SymbolTableEntry(tempResult, INTEGER, VARIABLE, "", YES, 1)});
	emit("mov", "[" + tempResult + "]", "eax", "; Store negated value into " + tempResult);
	
	// Update contentsOfAReg and operand1 stack
	contentsOfAReg = tempResult;
	pushOperand(tempResult);
	
	// Free temporary variable if necessary
	if (isTemporary(operand1)) 
		freeTemp();
}

void Compiler::emitNotCode(string operand1, string operand2)                // !op1
{
	// Check if operand exists in the symbol table and is of boolean type
	auto it = symbolTable.find(operand1);
	
	if (it == symbolTable.end()) 
	{
		processError("reference to undefined symbol in 'not' operation: " + operand1);
		return;
	}
	
	if (it->second.getDataType() != BOOLEAN) 
	{
		processError("'not' operation requires a boolean operand");
		return;
	}
	
	// Load operand into the A register if not already there
	if (contentsOfAReg != operand1)
	{
		emit("mov", "eax", "[" + it->second.getInternalName() + "]", "; Load " + operand1 + " into A register for 'not' operation");
		contentsOfAReg = operand1;
	}
	
	// Perform logical NOT operation
	emit("not", "eax", "", "; Perform logical NOT on the value in A register");
	
	// Assign result to a new temporary variable and update the symbol table
	string tempResult = getTemp();
	
	// insert(tempResult, BOOLEAN, VARIABLE, "", YES, 1);
	symbolTable.insert({tempResult, SymbolTableEntry(tempResult, BOOLEAN, VARIABLE, "", YES, 1)});
	emit("mov", "[" + tempResult + "]", "eax", "; Store negated value into " + tempResult);
	
	// Update contentsOfAReg and operand stack
	contentsOfAReg = tempResult;
	pushOperand(tempResult);
	
	// Free temporary variable if necessary
	if (isTemporary(operand1)) 
		freeTemp();
}


void Compiler::emitOrCode(string operand1, string operand2)             // op2 || op1
{
	// Check if operands exist in the symbol table and are of boolean type
	auto it1 = symbolTable.find(operand1);
	auto it2 = symbolTable.find(operand2);
	
	if (it1 == symbolTable.end() || it2 == symbolTable.end()) 
	{
		processError("reference to undefined symbol in 'or' operation");
		return;
	}
	
	if (it1->second.getDataType() != BOOLEAN || it2->second.getDataType() != BOOLEAN) 
	{
		processError("'or' operation requires boolean operands");
		return;
	}
	
	// Emit code to perform logical OR operation
	// Load lhs into A register
	emit("mov", "eax", "[" + it1->second.getInternalName() + "]", "; Load " + operand1 + " into A register");
	contentsOfAReg = operand1;
	
	// OR with rhs
	emit("or", "eax", "[" + it2->second.getInternalName() + "]", "; Logical OR " + operand1 + " with " + operand2);
	
	// Allocate labels for results
	string labelTrue = getLabel();
	string labelEnd = getLabel();
	
	// Jump to labelTrue if result is not zero (true)
	emit("jnz", labelTrue, "", "; Jump if result is true");
	
	// If result is zero (false), load 0 (false) into A register
	emit("mov", "eax", "0", "; Load 0 (false) into A register");
	emit("jmp", labelEnd, "", "; Jump to the end of OR operation");
	
	// True label: load -1 (true) into A register
	emit(labelTrue + ":", "mov eax", "-1", "; Load -1 (true) into A register");
	
	// End label
	emit(labelEnd + ":", "", "", "; End of OR operation");
	
	// Assign result to a temporary variable and push it onto the operand stack
	string tempResult = getTemp();
	// insert(tempResult, BOOLEAN, VARIABLE, "", YES, 1);
    symbolTable.insert({tempResult, SymbolTableEntry(tempResult, BOOLEAN, VARIABLE, "", YES, 1)});
	
	contentsOfAReg = tempResult;
	pushOperand(tempResult);
	
	// Free temporary variables if necessary
	if (isTemporary(operand1)) 
		freeTemp();
	
	if (isTemporary(operand2) && operand1 != operand2) 
		freeTemp();
}


void Compiler::emitInequalityCode(string operand1, string operand2)     // op2 != op1
{
	// Check if operands exist and have the same type
	auto it1 = symbolTable.find(operand1);
	auto it2 = symbolTable.find(operand2);
	
	if (it1 == symbolTable.end() || it2 == symbolTable.end()) {
		processError("reference to undefined symbol in inequality check");
		return;
	}
	
	if (it1->second.getDataType() != it2->second.getDataType()) {
		processError("incompatible types for inequality check");
		return;
	}
	
	// Load operand2 into the A register
	emit("mov", "eax", "[" + it2->second.getInternalName() + "]", "; Load " + operand2 + " into A register for inequality comparison");
	contentsOfAReg = operand2;
	
	// Compare operand1 with the value in the A register
	emit("cmp", "eax", "[" + it1->second.getInternalName() + "]", "; Compare " + operand1 + " with " + operand2);
	
	// Allocate labels for true (not equal) and false (equal) branches
	string labelTrue = getLabel();
	string labelFalse = getLabel();
	string labelEnd = getLabel();
	
	// Jump to labelTrue if not equal
	emit("jne", labelTrue, "", "; Jump to " + labelTrue + " if " + operand1 + " does not equal " + operand2);
	
	// False branch: load 0 (false) into A register if equal
	emit(labelFalse + ":", "mov eax", "0", "; Load 0 (false) into A register since values are equal");
	emit("jmp", labelEnd, "", "; Jump to the end of inequality check");
	
	// True branch: load -1 (true) into A register if not equal
	emit(labelTrue + ":", "mov eax", "-1", "; Load -1 (true) into A register since values are not equal");
	
	// End label
	emit(labelEnd + ":", "", "", "; End of inequality check");
	
	// Deassign temporaries if necessary
	if (isTemporary(operand1)) 
		freeTemp();
	
	if (isTemporary(operand2) && operand1 != operand2) 
		freeTemp();
	
	// Assign result to a temporary variable and push it onto the operand stack
	string tempResult = getTemp();
	// insert(tempResult, BOOLEAN, VARIABLE, "", YES, 1);
    symbolTable.insert({tempResult, SymbolTableEntry(tempResult, BOOLEAN, VARIABLE, "", YES, 1)});
	
	contentsOfAReg = tempResult;
	pushOperand(tempResult);
	
}

void Compiler::emitLessThanCode(string operand1, string operand2)       // op2 <  op1
{
	// Check if operands exist and are integers
	auto it1 = symbolTable.find(operand1);
	auto it2 = symbolTable.find(operand2);
	
	if (it1 == symbolTable.end() || it2 == symbolTable.end()) {
		processError("reference to undefined symbol in less than check");
		return;
	}
	
	if (it1->second.getDataType() != INTEGER || it2->second.getDataType() != INTEGER) {
		processError("less than comparison requires integer operands");
		return;
	}
	
	// Load operand2 into the A register
	emit("mov", "eax", "[" + it2->second.getInternalName() + "]", "; Load " + operand2 + " into A register for less than comparison");
	contentsOfAReg = operand2;
	
	// Subtract operand1 from the value in the A register
	emit("sub", "eax", "[" + it1->second.getInternalName() + "]", "; Subtract " + operand1 + " from " + operand2);
	
	// Allocate labels for true (less than) and false (not less than) branches
	string labelTrue = getLabel();
	string labelFalse = getLabel();
	string labelEnd = getLabel();
	
	// Jump to labelTrue if less than (if result of sub is negative)
	emit("jl", labelTrue, "", "; Jump to " + labelTrue + " if " + operand1 + " is less than " + operand2);
	
	// False branch: load 0 (false) into A register
	emit(labelFalse + ":", "mov eax", "0", "; Load 0 (false) into A register since " + operand1 + " is not less than " + operand2);
	emit("jmp", labelEnd, "", "; Jump to the end of less than check");
	
	// True branch: load -1 (true) into A register
	emit(labelTrue + ":", "mov eax", "-1", "; Load -1 (true) into A register since " + operand1 + " is less than " + operand2);
	
	// End label
	emit(labelEnd + ":", "", "", "; End of less than check");
	
	// Deassign temporaries if necessary
	if (isTemporary(operand1)) 
		freeTemp();
	
	if (isTemporary(operand2) && operand1 != operand2) 
		freeTemp();
	
	// Assign result to a temporary variable
	string tempResult = getTemp();
	
	//insert(tempResult, BOOLEAN, VARIABLE, "", YES, 1);
	symbolTable.insert({tempResult, SymbolTableEntry(tempResult, BOOLEAN, VARIABLE, "", YES, 1)});
	
	contentsOfAReg = tempResult;
	
	// Push the result onto the operand stack
	pushOperand(tempResult);
	
}

void Compiler::emitLessThanOrEqualToCode(string operand1, string operand2) // op2 <= op1
{
	// Check existence of operand1 and operand2 in the symbol table
	auto it1 = symbolTable.find(operand1);
	auto it2 = symbolTable.find(operand2);
	
	if (it1 == symbolTable.end() || it2 == symbolTable.end()) {
		processError("reference to undefined symbol: " + (it1 == symbolTable.end() ? operand1 : operand2));
		return;
	}
	
	// Check if the types of operands are integers
	if (it1->second.getDataType() != INTEGER || it2->second.getDataType() != INTEGER) {
		processError("less than or equal to comparison requires operands of integer type");
		return;
	}
	
	// Load operand2 into the A register if it is not already there
	if (contentsOfAReg != operand2) {
		emit("mov", "eax", "[" + it2->second.getInternalName() + "]", "; Load " + operand2 + " into A register for <= comparison");
		contentsOfAReg = operand2;
	}
	
	// Subtract operand1 from operand2
	emit("sub", "eax", "[" + it1->second.getInternalName() + "]", "; Compare " + operand1 + " with " + operand2 + " for <=");
	
	// Allocate labels for true (less than or equal) and false branches
	string labelTrue = getLabel();
	string labelFalse = getLabel();
	string labelEnd = getLabel();
	
	// Jump to labelTrue if less than or equal
	emit("jle", labelTrue, "", "; Jump to " + labelTrue + " if " + operand1 + " is less than or equal to " + operand2);
	
	// False branch: load 0 (false) into A register
	emit(labelFalse + ":", "mov eax", "0", "; Load 0 (false) into A register since " + operand1 + " is not less than or equal to " + operand2);
	emit("jmp", labelEnd, "", "; Jump to the end of <= check");
	
	// True branch: load -1 (true) into A register
	emit(labelTrue + ":", "mov eax", "-1", "; Load -1 (true) into A register since " + operand1 + " is less than or equal to " + operand2);
	
	// End label
	emit(labelEnd + ":", "", "", "; End of <= check");
	
	// Deassign temporaries if necessary
	if (isTemporary(operand1)) 
		freeTemp();
	
	if (isTemporary(operand2) && operand1 != operand2) 
		freeTemp();
	
	// Assign result to a temporary variable
	string tempResult = getTemp();
	
	// insert(tempResult, BOOLEAN, VARIABLE, "", YES, 1);
	symbolTable.insert({tempResult, SymbolTableEntry(tempResult, BOOLEAN, VARIABLE, "", YES, 1)});
	contentsOfAReg = tempResult;
	
	// Push the result onto the operand stack
	pushOperand(tempResult);

}

void Compiler::emitGreaterThanCode(string operand1, string operand2)    // op2 >  op1
{
	// Check existence of operand1 and operand2 in the symbol table
	auto it1 = symbolTable.find(operand1);
	auto it2 = symbolTable.find(operand2);
	
	if (it1 == symbolTable.end() || it2 == symbolTable.end()) {
		processError("reference to undefined symbol: " + (it1 == symbolTable.end() ? operand1 : operand2));
		return;
	}
	
	// Check if the types of operands are integers
	if (it1->second.getDataType() != INTEGER || it2->second.getDataType() != INTEGER) {
		processError("greater than comparison requires operands of integer type");
		return;
	}
	
	// Load operand2 into the A register if it is not already there
	if (contentsOfAReg != operand2) {
		emit("mov", "eax", "[" + it2->second.getInternalName() + "]", "; Load " + operand2 + " into A register for > comparison");
		contentsOfAReg = operand2;
	}
	
	// Subtract operand1 from operand2
	emit("sub", "eax", "[" + it1->second.getInternalName() + "]", "; Compare " + operand1 + " with " + operand2 + " for >");
	
	// Allocate labels for true (greater than) and false branches
	string labelTrue = getLabel();
	string labelFalse = getLabel();
	string labelEnd = getLabel();
	
	// Jump to labelTrue if greater than
	emit("jg", labelTrue, "", "; Jump to " + labelTrue + " if " + operand1 + " is greater than " + operand2);
	
	// False branch: load 0 (false) into A register
	emit(labelFalse + ":", "mov eax", "0", "; Load 0 (false) into A register since " + operand1 + " is not greater than " + operand2);
	emit("jmp", labelEnd, "", "; Jump to the end of > check");
	
	// True branch: load -1 (true) into A register
	emit(labelTrue + ":", "mov eax", "-1", "; Load -1 (true) into A register since " + operand1 + " is greater than " + operand2);
	
	// End label
	emit(labelEnd + ":", "", "", "; End of > check");
	
	// Deassign temporaries if necessary
	if (isTemporary(operand1)) 
		freeTemp();
	
	if (isTemporary(operand2) && operand1 != operand2) 
		freeTemp();
	
	// Assign result to a temporary variable
	string tempResult = getTemp();
	// insert(tempResult, BOOLEAN, VARIABLE, "", YES, 1);
    symbolTable.insert({tempResult, SymbolTableEntry(tempResult, BOOLEAN, VARIABLE, "", YES, 1)});
	contentsOfAReg = tempResult;
	
	// Push the result onto the operand stack
	pushOperand(tempResult);
}

void Compiler::emitGreaterThanOrEqualToCode(string operand1, string operand2) // op2 >= op1
{
	// Check existence of operand1 and operand2 in the symbol table
	auto it1 = symbolTable.find(operand1);
	auto it2 = symbolTable.find(operand2);
	
	if (it1 == symbolTable.end() || it2 == symbolTable.end()) {
		processError("reference to undefined symbol: " + (it1 == symbolTable.end() ? operand1 : operand2));
		return;
	}
	
	// Check if the types of operands are integers
	if (it1->second.getDataType() != INTEGER || it2->second.getDataType() != INTEGER) {
		processError("greater than or equal to comparison requires operands of integer type");
		return;
	}
	
	// Load operand2 into the A register if it is not already there
	if (contentsOfAReg != operand2) {
		emit("mov", "eax", "[" + it2->second.getInternalName() + "]", "; Load " + operand2 + " into A register for >= comparison");
		contentsOfAReg = operand2;
	}
	
	// Subtract operand1 from operand2
	emit("sub", "eax", "[" + it1->second.getInternalName() + "]", "; Compare " + operand1 + " with " + operand2 + " for >=");
	
	// Allocate labels for true (greater than or equal) and false branches
	string labelTrue = getLabel();
	string labelFalse = getLabel();
	string labelEnd = getLabel();
	
	// Jump to labelTrue if greater than or equal
	emit("jge", labelTrue, "", "; Jump to " + labelTrue + " if " + operand1 + " is greater than or equal to " + operand2);
	
	// False branch: load 0 (false) into A register
	emit(labelFalse + ":", "mov eax", "0", "; Load 0 (false) into A register since " + operand1 + " is not greater than or equal to " + operand2);
	emit("jmp", labelEnd, "", "; Jump to the end of >= check");
	
	// True branch: load -1 (true) into A register
	emit(labelTrue + ":", "mov eax", "-1", "; Load -1 (true) into A register since " + operand1 + " is greater than or equal to " + operand2);
	
	// End label
	emit(labelEnd + ":", "", "", "; End of >= check");
	
	// Deassign temporaries if necessary
	if (isTemporary(operand1)) 
		freeTemp();
	
	if (isTemporary(operand2) && operand1 != operand2) 
		freeTemp();
	
	// Assign result to a temporary variable
	string tempResult = getTemp();
	
	//insert(tempResult, BOOLEAN, VARIABLE, "", YES, 1);
	symbolTable.insert({tempResult, SymbolTableEntry(tempResult, BOOLEAN, VARIABLE, "", YES, 1)});
	
	contentsOfAReg = tempResult;
	
	// Push the result onto the operand stack
	pushOperand(tempResult);
}
