#include "project.h"

project::project()
{
	currentIndex = 0;
	error = stderr;
        debug = stderr;
        output = stdout;
}

project::~project()
{
	for(unsigned long i=0; i < symbols.size(); i++)
	{
		delete symbols[i];
	}
	symbols.clear();
}

void project::openFiles(const char* errorFile, const char* debugFile)
{
	if(errorFile && *errorFile)
        {
                error = fopen(errorFile, "w+");
                if(!error)
                {
                        error = stderr;
                }
        }

        if(debugFile && *debugFile)
        {
                debug = fopen(debugFile, "w+");
                if(!debug)
                {
                        debug = stderr;
                }
        }
}

symbol* project::compile(const char *file, const char* sourceFile, const char* errorFile, const char* debugFile)
{
	openFiles(errorFile, debugFile);
        return compile(file, sourceFile, error, debug);
}

symbol* project::compile(const char *file, const char* sourceFile)
{
	if(getFileSymbol(file)) return files[file];
	symbol* s = new symbol();
        s->proj = this;
        symbols.push_back(s);
	files[file] = s;
        s->compile(file, sourceFile, error, debug);
        return s;
}

symbol* project::getFileSymbol(const char *file)
{
	if(files.count(file)) return files[file];
	return NULL;
}

symbol* project::compile(const char *file, const char* sourceFile, FILE* errorFile, FILE* debugFile)
{
	if(getFileSymbol(file)) return files[file];
	symbol* s = new symbol();
        s->proj = this;
        symbols.push_back(s);
	files[file] = s;
	printf("files stack:");
	for(std::map<string, symbol*>::iterator it = files.begin(); it != files.end(); it++)
	{
		printf(" %s", it->first.c_str());
	}
	printf("\n");
        s->compile(file, sourceFile, errorFile, debugFile);
	return s;
}

void project::printTree(FILE* fd, int depth)
{
	for(unsigned long i=0; i < symbols.size(); i++)
	{
		symbols[i]->printTree(fd, depth);
	}
}
