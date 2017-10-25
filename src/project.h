#ifndef PROJECT_H
#define PROJECT_H

#include "symbol.h"

class project
{
public:
	project();
	~project();
	FILE* error;
        FILE* debug;
        FILE* output;
	unsigned long currentIndex;
	std::vector<symbol*> symbols;
	std::map<string, symbol*> files;
	symbol* getFileSymbol(const char *file);
	void printTree(FILE* fd=NULL, int depth=0);
	void openFiles(const char* errorFile, const char* debugFile);
	symbol* compile(const char *file, const char* sourceFile, FILE* err, FILE* dbg);
	symbol* compile(const char *file, const char* sourceFile, const char* errorFile, const char* debugFile);
	symbol* compile(const char *file, const char* sourceFile);
};

#endif
