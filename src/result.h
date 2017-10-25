#ifndef RESULTS_H
#define RESULTS_H

#include <stdio.h>
#include <sx/sx>
#include <sx/strings.h>

class symbol;

#define OPERATOR_DIRECTIONS std::map<unsigned char, symbol*>
#define OPERATORS std::map<string, OPERATOR_DIRECTIONS >
#define OPERATIONS std::vector<operation>
#define OPERATION_ORDERS std::map<long, std::vector<unsigned long> >

#define OP_RTL 1
#define OP_LTR 0
#define OP_SUB 2

#include "sscope.h"

struct RESULT
{
	RESULT();
	RESULT(symbol* parent, STRINGS &tokens);
	~RESULT();
	void reset();
	//symbol* sym(symbol* s);
	//RESULT resolveExpression(STRINGS &statement);
	RESULT* parse(symbol* parent, STRINGS &tokens);
	RESULT* parse(symbol* parent, STRINGS &tokens, unsigned long &i);
	RESULT* parse(symbol* parent, const char* buffer);
	STRINGS parseNextSymbol(unsigned long &i);

	STRINGS statement;
	symbol* parent;
	symbol* sym;
	string val;
	string name;
	string resolved;
	bool isOperator;
	bool isConst;
	bool isNull;
	bool isPointer;
	char pointer;
	char reference;
	long l;
	int type;
	symbol* datatype;
	OPERATORS operators;
};

struct RESULTSET
{
        RESULTSET();
        RESULTSET(symbol* parent, STRINGS &tokens);
        ~RESULTSET();
        void reset();
        RESULTSET* parse(symbol* parent, STRINGS &tokens, unsigned long start=0);
        RESULTSET* parse(symbol* parent, const char* buffer);
        std::vector<RESULT> results;
        RESULT& first();
	RESULT& last();
        RESULT& get(unsigned long i=0);
        unsigned long size();

	RESULT& operator[](unsigned long i)
        {
                return get(i);
        }

        symbol* parent;
	STRINGS statement;
};

struct operation
{
        operation()
        {
                reset();
        }
        void reset()
        {
		base = NULL;
		pointer = 0;
                next = 0;
                prev = 0;
		params[0].reset();
		params[1].reset();
		op = NULL;
		cache = "";
		datatype = NULL;
		isConst = false;
		prefix = false;
		isOp = false;
		param.clear();
		sym = NULL;
		scope.reset();
		anonymous = false;
        }

	string& renderMember(string& s, operation* right = NULL)
	{
		/*if(pointer > 1)
		{
			for(int i=1; i < pointer; i++) s += "*";
		}
		s += name.c_str();
		if(pointer || reference)
		{
			s += "->";
		}
		else
		{
			s += ".";
		}
		if(right) right->render(s, NULL);*/
		return s;
	}
	bool anonymous;
        symbol* op;
	symbol* sym;
	symbol* datatype;
	char pointer;
        RESULT params[2];
        STRINGS param;
        string cache;
	sscope scope;
	symbol* base;
        bool prefix;
	unsigned char isOp;
	bool isConst;
        unsigned long prev;
        unsigned long next;
};

#include "symbol.h"

#endif
