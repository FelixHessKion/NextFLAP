/********************************************************/
/* Oscar Sapena Vercher - DSIC - UPV                    */
/* April 2022                                           */
/********************************************************/
/* Syntactic analyzer for the domain and problem files. */
/* Splits the text into a list of syntatic tokens.      */
/********************************************************/

#include "syntaxAnalyzer.h"
#include <fstream>
#include <sstream>
#include <cstring>
#include <algorithm>
#include <cstdarg>
#include "../utils/utils.h"
using namespace std;

/********************************************************/
/* CLASS: Token (syntatic tokens)                       */
/********************************************************/

// Creates a new token
Token::Token(Symbol s) {
    symbol = s;
}

// Creates a numeric token
Token::Token(float v) {
    symbol = Symbol::NUMBER;
    value = v;
}

// Creates a string token
Token::Token(Symbol s, string const& desc) {
    symbol = s;
    description = desc;
}

// Returns a string representation of this token
string Token::toString() {
    if (symbol == Symbol::NUMBER) return std::to_string(value);
    else if (symbol == Symbol::NAME || symbol == Symbol::VARIABLE) return description;
    else return symbolDescriptions[symbol];
}

/********************************************************/
/* CLASS: SyntaxAnalyzer (Syntatic analyzer)            */
/********************************************************/

SyntaxAnalyzer::SyntaxAnalyzer()
{
    buffer = NULL;
}

// Creates a new syntactic analyzer for parsing a given file
SyntaxAnalyzer::SyntaxAnalyzer(char* fileName) {
    this->fileName = fileName;
    ifstream in(fileName);
    if (in.fail()) {
        throwError("File not found: " + std::string(fileName));
    }
    string contents((istreambuf_iterator<char>(in)),
        istreambuf_iterator<char>());
    std::transform(contents.begin(), contents.end(), contents.begin(), ::tolower);
    bufferLength = contents.length() + 1;
    buffer = std::make_unique<char[]>(bufferLength);
    strcpy(buffer.get(), contents.c_str());
    tokenIndex = 0;
    lineNumber = 1;
    position = 0;
    tokens.reserve(8096);
    for (unsigned int i = 0; i < numSymbols; i++) {
        if (symbolNames[i] != nullptr)
            symbols[symbolNames[i]] = (Symbol)i;
    }
}

SyntaxAnalyzer::SyntaxAnalyzer(std::string &pddlStr){
    std::stringstream in(pddlStr);
    string contents((istreambuf_iterator<char>(in)),
        istreambuf_iterator<char>());
    std::transform(contents.begin(), contents.end(), contents.begin(), ::tolower);
    bufferLength = contents.length() + 1;
    buffer = std::make_unique<char[]>(bufferLength);
    strcpy(buffer.get(), contents.c_str());
    tokenIndex = 0;
    lineNumber = 1;
    position = 0;
    tokens.reserve(8096);
    for (unsigned int i = 0; i < numSymbols; i++) {
        if (symbolNames[i] != nullptr)
            symbols[symbolNames[i]] = (Symbol)i;
    }
}

// Disposes the syntatic analyzer
SyntaxAnalyzer::~SyntaxAnalyzer() {
    tokens.clear();
    symbols.clear();
}

// Returns the next token in the file
std::shared_ptr<Token> SyntaxAnalyzer::nextToken() {
    if (tokenIndex < 0) {
        tokenIndex++;
        return tokens[tokens.size() + tokenIndex - 1];
    }
    else {
        skipSpaces();   // Skip comments and spaces
        while (position < bufferLength) {
            if (buffer[position] == ';' || buffer[position] == '\\') {
                while (position < bufferLength && buffer[position] != '\n')
                    position++;
                skipSpaces();
            }
            else break;
        }
        std::shared_ptr<Token> token;
        if (position < bufferLength) token = matchToken();
        else token = nullptr;
        if (token == nullptr)
            notifyError("Reached end of file");
        tokens.push_back(token);
        return token;
    }
}

// Computes the type of the next token
std::shared_ptr<Token> SyntaxAnalyzer::matchToken() {
    std::shared_ptr<Token> token = nullptr;
    switch (buffer[position]) {
    case '(': token = std::make_shared<Token>(Symbol::OPEN_PAR);  break;
    case ')': token = std::make_shared<Token>(Symbol::CLOSE_PAR); break;
    case ':': token = std::make_shared<Token>(Symbol::COLON);     break;
    case '-': token = std::make_shared<Token>(Symbol::MINUS);     break;
    case '+': token = std::make_shared<Token>(Symbol::PLUS);      break;
    case '/': token = std::make_shared<Token>(Symbol::DIV);       break;
    case '*': token = std::make_shared<Token>(Symbol::PROD);      break;
    case '=': token = std::make_shared<Token>(Symbol::EQUAL);     break;
    case '>':
        if (buffer[position + 1] == '=') {
            token = std::make_shared<Token>(Symbol::GREATER_EQ);
            position++;
        }
        else token = std::make_shared<Token>(Symbol::GREATER);
        break;
    case '<':
        if (buffer[position + 1] == '=') {
            token = std::make_shared<Token>(Symbol::LESS_EQ);
            position++;
        }
        else token = std::make_shared<Token>(Symbol::LESS);
        break;
    case '#':
        if (buffer[position + 1] == 't') {
            token = std::make_shared<Token>(Symbol::SHARP_T);
            position++;
        }
        else notifyError("#t variable expected");
        break;
    }
    if (token != nullptr) {
        position++;
        float value;
        if (matchNumber(&value)) token = std::make_shared<Token>(-value);
    }
    else {
        float value;
        if (matchNumber(&value)) token = std::make_shared<Token>(value);
        else {
            int start = position++;
            while (position < bufferLength &&
                ((buffer[position] >= 'a' && buffer[position] <= 'z') ||
                    (buffer[position] >= '0' && buffer[position] <= '9') ||
                    buffer[position] == '-' || buffer[position] == '_'))
                position++;
            string description(&buffer[start], position - start);
            if (description.at(0) == '?') token = std::make_shared<Token>(Symbol::VARIABLE, description);
            else {
                unordered_map<string, Symbol>::const_iterator index = symbols.find(description);
                if (index == symbols.end()) {
                    token = std::make_shared<Token>(Symbol::NAME, description);
                }
                else {
                    token = std::make_shared<Token>(index->second, description);
                }
            }
        }
    }
    return token;
}

// Checks if the next token is a number
bool SyntaxAnalyzer::matchNumber(float* value) {
    if (position >= bufferLength || buffer[position] < '0' || buffer[position] > '9')
        return false;
    *value = 0;
    while (position < bufferLength && buffer[position] >= '0' && buffer[position] <= '9') {
        *value = *value * 10 + (int)buffer[position] - (int)'0';
        position++;
    }
    if (position < bufferLength && buffer[position] == '.') {
        float decimals = 0, digits = 1;
        position++;
        while (position < bufferLength && buffer[position] >= '0' && buffer[position] <= '9') {
            decimals = decimals * 10 + (int)buffer[position] - (int)'0';
            digits *= 10;
            position++;
        }
        *value += decimals / digits;
    }
    return true;
}

// Skips the spaces in the text
void SyntaxAnalyzer::skipSpaces() {
    while (position < bufferLength && buffer[position] <= ' ') {
        if (buffer[position] == '\n') lineNumber++;
        position++;
    }
}

// Reads an open parenthesis
void SyntaxAnalyzer::openPar() {
    std::shared_ptr<Token> res = nextToken();
    if (!isSym(res, Symbol::OPEN_PAR))
        notifyError("Open parenthesis expected");
}

// Reads a close parenthesis
void SyntaxAnalyzer::closePar() {
    std::shared_ptr<Token> res = nextToken();
    if (!isSym(res, Symbol::CLOSE_PAR))
        notifyError("Close parenthesis expected");
}

// Reads a colon
void SyntaxAnalyzer::readColon() {
    std::shared_ptr<Token> res = nextToken();
    if (!isSym(res, Symbol::COLON))
        notifyError("Colon expected but token '" + res->toString() + "' found");
}

// Raises a parse error
void SyntaxAnalyzer::notifyError(const std::string& msg) {
    throw new std::invalid_argument(msg);
}

// Returns the next token if its type is equal to the given one
std::shared_ptr<Token> SyntaxAnalyzer::readSymbol(Symbol s) {
    return readSymbol(1, s);
}

// Returns the next token if its type is equal to one in the given list of types
std::shared_ptr<Token> SyntaxAnalyzer::readSymbol(int numSymbols, ...) {
    std::shared_ptr<Token> res = nextToken();
    va_list list;
    va_start(list, numSymbols);
    bool valid = false;
    for (int i = 0; i < numSymbols && !valid; i++) {
        int sym = va_arg(list, int);
        if (isSym(res, (Symbol)sym)) valid = true;
    }
    va_end(list);
    if (!valid) {
        notifyError("Unexpected token '" + res->toString() + "'");
    }
    return res;
}

// Returns the description of the next token if it is a name
string SyntaxAnalyzer::readName() {
    std::shared_ptr<Token> res = nextToken();
    if (!isSym(res, Symbol::NAME) && !isSym(res, Symbol::AT) && !isSym(res, Symbol::OVER)
        && !isSym(res, Symbol::OBJECTS) && !isSym(res, Symbol::CONSTRAINTS)) {
        notifyError("Name expected, but token '" + res->toString() + "' found");
    }
    return res->description;
}
