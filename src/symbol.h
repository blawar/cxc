#ifndef SYMBOLS_H
#define SYMBOLS_H

#include <stdio.h>
#include <sx/sx>
#include <sx/strings.h>
#include "result.h"
#include "project.h"

#define DEBUG 6
#define REF_NONE 0
#define REF_PTR 1
#define REF_FULL 2

enum
{
	UNKNOWN		= 0,
	IMPORT		= 1,
	EXTERN		= 2,
	NUMERIC		= 3,
	STRING		= 4,
	POD		= 5,
	STRUCT		= 6,
	CLASS		= 7,
	FUNCTION	= 8,
	VARIABLE	= 9,
	SCOPE		= 10,
	STATEMENT	= 11,
	RETURN		= 12,
	CONTROL		= 13,
	GROUP		= 14,
	MACRO		= 15,
	TEMPLATE	= 16,
	PRECEDENCE	= 17
};

#include "sscope.h"

class project;

class symbol
{
public:
	symbol();

	symbol(symbol &sym, symbol* p);

	symbol(const symbol &sym);

	symbol & operator=(const symbol &copy);

	symbol(int type, const char* name, symbol* parent = NULL);

	symbol(int type, symbol* datatype, const char* name, symbol* parent = NULL);

	symbol* init();
	symbol* init(int type, const char* name, symbol* parent = NULL);

	~symbol();

	bool isDatatype();
	bool isDatatype(symbol &sym);

	symbol* clone(symbol* parent=NULL);

	string& getDatatype(bool convert=false);
	string& getDatatype(long &pointer, bool convert=false);
	string& getDatatype(string &buffer, const char* datatype, char pointer);
	string& getReferenceDatatype(bool convert=false);

	STRINGS* findDefine(string &name);

	symbol* findSymbol(const char* name, bool strict = true, sscope* scope = NULL, sscope* actualScope = NULL);
	symbol* findSimpleSymbol(const char* name, bool strict = true, bool scanImports=true);
	symbol* findLocalSymbol(const char* name, bool strict = true);

	symbol* findFunction(const char* name, const char* returnType, std::vector<const char*> &args, bool recursive=true, long depth=0, bool fuzzy=false, bool scanImports=true);

	symbol* findFunction(const char* name, const char* returnType, bool recursive=true, bool scanImports=true);

	symbol* findFunction(const char* name, const char* returnType, const char* arg1, bool recursive=true, bool scanImports=true);

	symbol* findFunction(const char* name, const char* returnType, const char* arg1, const char* arg2, bool recursive=true, bool scanImports=true);

	symbol* findFunction(const char* name, const char* returnType, const char* arg1, const char* arg2, const char* arg3, bool recursive=true, bool scanImports=true);

	symbol* findFunction(const char* name, const char* returnType, const char* arg1, const char* arg2, const char* arg3, const char* arg4, bool recursive=true, bool scanImports=true);

	symbol* closestStruct();
	void setParents();

	std::vector<symbol*> parseParameters(STRINGS &tokens, unsigned long &i, unsigned long end=0);
	void getStatementEnd(STRINGS &tokens, unsigned long i, unsigned long &end, bool &hasBody, bool ignoreCommas = true);

	symbol parseStatement(STRINGS &tokens, unsigned long &i);

	//RESULT resolveExpression(string &buffer, symbol* p = NULL, long depth=0);
	//RESULT resolveExpression(unsigned long &i, string &buffer, symbol* p = NULL, long depth=0);
	//STRINGS resolveOrder(unsigned long pos, symbol* p = NULL);

	RESULTSET resolveStatement();

	RESULTSET resolveStatements(bool processBody = true);

	symbol* createChild();
	void createChild(symbol &sym);

	string render(unsigned char showref=REF_PTR, symbol* classBase = NULL, symbol* root = NULL, sscope* scope = NULL);

	string& render(string &buffer, unsigned char showref=REF_PTR, symbol* classBase = NULL, symbol* root = NULL, sscope* scope = NULL);
	string& render(sscope* scope, string& s, symbol* root = NULL);
	string& renderMember(string& s, symbol* member = NULL);

	unsigned long incRefCount(symbol* root);
	symbol* getRoot();
	void getOperators(OPERATORS &operators);
	STRINGS parseNextSymbol(unsigned long &i, STRINGS &statement);

	void setIndex();

	void setScope();
	void pushScope(sscope *s);

	void printTree(FILE* fd=NULL, int depth=0);
	inline void addChild(symbol &sym);
	void parse(STRINGS & tokens, unsigned long &i);
	int resolveInheritance(STRINGS &tokens);
	void renderDatatypes(FILE* output, int depth=0);
	void renderDatatypeBodies(FILE* output, int depth=0);
	void renderDefinitions(FILE* output, int depth=0);
	void renderTemplateDefinitions(FILE* output, int depth=0);
	void compile(const char *file, const char* sourceFile=NULL, const char* errorFile=NULL, const char* debugFile=NULL);
	void compile(const char *file, const char* sourceFile, FILE* errorFile, FILE* debugFile);
	void loadLibrary(const char *file, symbol &parent, bool include);
	void print();
	void printHeaders(FILE* output);

	project* proj;
	bool isMacro;
	bool isOperator;
	bool verbatim;
	unsigned char rtl;
	bool isTemplate;
	bool hidden;
	long precedence;
	symbol* parent;
	symbol* datatype;
	unsigned char type;
	bool external, reference;
	char pointer;
	bool isInline;
	bool isImport;
	unsigned long referenced;
	unsigned long depth;
	unsigned long index;
	unsigned long functionCount;
	string name, datatype_cache;
	STRINGS statement;
	STRINGS orderedStatement;
	FILE* error;
	FILE* debug;
	FILE* output;
	std::vector < symbol* > symbols;
        std::vector < string > extends;
	std::vector < symbol* > arguments;
	std::vector < symbol* > templates;
	std::vector < string > imports;
	std::vector < sscope > scopes;
	std::map < string, STRINGS > defines;
	std::map<symbol*, unsigned long> references;
};

char* datatypeBase(const char* s, string* out);
string ultoa(long value, unsigned int base);

#endif
