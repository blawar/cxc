#include <stdio.h>
#include <sx/sx>
#include <sx/strings.h>
#include "symbol.h"
#include <boost/filesystem.hpp>

void renderC(FILE* output, std::vector < symbol > &symbols, string prefix = "", int depth=0);
void renderC(FILE* output, symbol & sym, string prefix = "", int depth=0);
void renderCDef(FILE* output, symbol & sym,  string prefix = "", int depth = 0);
symbol nullSymbol;

long pointerCount(const char* s)
{
	long r = 0;
	if(!s) return 0;

	while(*s && *s != '*') s++;
	while(*s && *s == '*')
	{
		r++;
		s++;
	}
	return r;
}

char* datatypeBase(const char* s, string* out)
{
	*out = s;
	char* n = (char*)out->c_str();
	while(*n)
	{
		if(*n == '*') break;
		n++;
	}
	*n = NULL;
	return n;
}

string ultoa(long value, unsigned int base)
{
	char* digitMap = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_$";
	if(base == 10) digitMap = "0123456789";
	else if(base == 16) digitMap = "0123456789ABCDEF";

	string buf;
	// Guard:

	if (base == 0 || base > 64) {

		// Error: may add more trace/log output here

		return buf;

	}



	// Take care of negative int:

	std::string sign;

	int _value = value;



	// Check for case when input is zero:

	if (_value == 0) return "0";



	// Translating number to string with base:

	for (int i = 31; _value && i ; --i) {

		buf = digitMap[ _value % base ] + buf;

		_value /= base;

	}

	return buf;

}


symbol::symbol() : statement()
{
	init();
}

symbol::symbol(symbol &sym, symbol* p) : statement()
{
	init();
	*this = sym;
	parent = p;
	this->setParents();
}

symbol::symbol(const symbol &sym)
{
	//init();
	//*this = sym;
	operator=(sym);
}

symbol & symbol::operator=(const symbol &copy)
{
	init();
	precedence = copy.precedence;
	isOperator = copy.isOperator;
	isMacro = copy.isMacro;
	rtl = copy.rtl;
	parent = copy.parent;
	depth = copy.depth;
	type = copy.type;
	isTemplate = copy.isTemplate;
	proj = copy.proj;
	hidden = copy.hidden;
	external = copy.external;
	reference = copy.reference;
	pointer = copy.pointer;
	name = copy.name.c_str();
	datatype = copy.datatype;
	datatype_cache = copy.datatype_cache.c_str();
	statement = copy.statement;
	defines = copy.defines;
	extends = copy.extends;
	imports = copy.imports;
	isImport = copy.isImport;
	isInline = copy.isInline;
	verbatim = copy.verbatim;
	referenced = copy.referenced;
	references = copy.references;
	error = copy.error;
	debug = copy.debug;
	output = copy.output;

	symbols.clear();
	arguments.clear();
	templates.clear();

	for(unsigned long i=0; i < copy.symbols.size(); i++)
	{
		symbols.push_back(new symbol(*copy.symbols[i]));
	}

	for(unsigned long i=0; i < copy.arguments.size(); i++)
	{
		arguments.push_back(new symbol(*copy.arguments[i]));
	}

	for(unsigned long i=0; i < copy.templates.size(); i++)
	{
		templates.push_back(new symbol(*copy.templates[i]));
	}
	this->setParents();

	return *this;
}

symbol::symbol(int type, const char* name, symbol* parent) : statement()
{
	init();
	external = pointer = reference = 0;
	this->parent = NULL;//parent;
	this->type = type;
	this->name = name;
	if(parent) error = parent->error;
}

symbol::symbol(int type, symbol* datatype, const char* name, symbol* parent) : statement()
{
	init();
	external = pointer = reference = 0;
	this->parent = NULL;//parent;
	this->type = type;
	this->name = name;
	this->datatype = datatype;
	if(parent) error = parent->error;
}

symbol* symbol::init()
{
	//fprintf(debug, "created %x\n", this);
	datatype = NULL;
	precedence = 0;
	isOperator = false;
	rtl = 0;
	isMacro = false;
	isTemplate = false;
	hidden = false;
	external = pointer = reference = 0;
	parent = NULL;
	type = 0;
	depth = 0;
	functionCount = 0;
	index = 0;
	isInline = false;
	verbatim = false;
	error = stderr;
	debug = stderr;
	output = stdout;
	isImport = false;
	referenced = 0;
	proj = NULL;
	return this;
}

symbol* symbol::init(int type, const char* name, symbol* parent)
{
	external = pointer = reference = 0;
	this->parent = NULL;//parent;
	this->type = type;
	this->name = name;
	this->depth = 0;
	this->functionCount = 0;
	this->index = 0;
	this->isInline = false;
	return this;
}

symbol::~symbol()
{
	for(unsigned long i=0; i < symbols.size(); i++)
	{
		delete symbols[i];
	}
	symbols.clear();

	for(unsigned long i=0; i < arguments.size(); i++)
        {
                delete arguments[i];
        }
        arguments.clear();

	for(unsigned long i=0; i < templates.size(); i++)
        {
                delete templates[i];
        }
        templates.clear();
	//fprintf(debug, "destroyed %x\n", this);
}

bool symbol::isDatatype()
{
	return (type >= POD && type <= CLASS);
}

bool symbol::isDatatype(symbol &sym)
{
	return (sym.type >= POD && sym.type <= CLASS);
}

string& symbol::getDatatype(string &buffer, const char* datatype, char pointer)
{
	buffer = datatype;
	for(char i=0; i < pointer; i++) buffer += '*';
	return buffer;
}

string& symbol::getDatatype(bool convert)
{
	if(datatype)
	{
		datatype_cache = datatype->name;
		for(char i=0; i < pointer; i++) datatype_cache += '*';
		if(reference) datatype_cache += '&';
	}
	else
	{
		if(statement.size())
		{
			if(convert)
			{
				pointer = 0;
				datatype = parent->findSymbol(statement[0]);
			}
			datatype_cache = statement[0];
			for(unsigned int i=1; i < statement.size(); i++)
			{
				if(strcmp(statement[i], "*")) break;
				datatype_cache += '*';
				if(convert) pointer++;
			}
		}
		else datatype_cache = "";
	}
	return datatype_cache;
}

string& symbol::getDatatype(long &pointer, bool convert)
{
	if(datatype)
	{
		//datatype_cache = "";
		datatype_cache = datatype->name;
		//datatype->render(datatype_cache);
		for(char i=0; i < this->pointer; i++) datatype_cache += '*';
		//if(reference) datatype_cache += '&';
		pointer = this->pointer;
	}
	else
	{
		if(statement.size())
		{
			pointer = 0;
			if(convert) datatype = parent->findSymbol(statement[0]);
			datatype_cache = statement[0];
			for(unsigned int i=1; i < statement.size(); i++)
			{
				if(strcmp(statement[i], "*")) break;
				datatype_cache += '*';
				pointer++;
			}
		}
		else datatype_cache = "";
	}
	if(convert) this->pointer = (char)pointer;
	return datatype_cache;
}

string& symbol::getReferenceDatatype(bool convert)
{
	getDatatype(convert);
	datatype_cache += "*";
	return datatype_cache;
}

symbol* symbol::findSimpleSymbol(const char* name, bool strict, bool scanImports)
{
	for(unsigned long i=0; i < symbols.size(); i++)
        {
                if(!symbols[i]->name.compare(name))
                {
                        return symbols[i];
                }
        }

	if(parent == NULL)
        {
		if(proj && scanImports)
                {
                        fprintf(debug, "searching project tree\n");
                        symbol* r;
                        symbol* root = getRoot();
                        for(unsigned long i=0; i < proj->symbols.size(); i++)
                        {
                                if(proj->symbols[i] == root) continue;
                                r = proj->symbols[i]->findSimpleSymbol(name, false, false);
                                if(r)
                                {
                                        return r;
                                }
                        }
                }

                if(strict)
                {
                        fprintf(debug, "failed to find %s\n", name);
                        throw "Failed to find symbol";
                }
                return NULL;
        }

        if(type == FUNCTION)
        {
                for(unsigned long i=0; i < arguments.size(); i++)
                {
                        if(!arguments[i]->name.compare(name))
                        {
                                return arguments[i];
                        }
                }
        }
        //fprintf(debug, "%x -> %x for %s\n", this, parent, name);
        if(parent->parent == this)
        {
                fprintf(debug, "%x -> %x for %s\n", this, parent, name);
                throw "loop";
        }
        return parent->findSimpleSymbol(name, strict);
}

symbol* symbol::findLocalSymbol(const char* name, bool strict)
{
        for(unsigned long i=0; i < symbols.size(); i++)
        {
                if(!symbols[i]->name.compare(name))
                {
                        return symbols[i];
                }
        }

        if(type == FUNCTION)
        {
                for(unsigned long i=0; i < arguments.size(); i++)
                {
                        if(!arguments[i]->name.compare(name))
                        {
                                return arguments[i];
                        }
                }
        }

	if(strict)
	{
		fprintf(debug, "failed to find %s\n", name);
		throw "Failed to find symbol";
	}
	return NULL;
}

symbol* symbol::findSymbol(const char* name, bool strict, sscope* scope, sscope* actualScope)
{
	if(!this) return NULL;
	if(!scope)
	{
		if(actualScope) actualScope->type = SCOPE_NONE;
		symbol *r;
		for(int i=scopes.size()-1; i > -1; i--)
		{
			r = findSymbol(name, false, &scopes[i], actualScope);
			if(r)
			{
				//if(actualScope) *actualScope = scopes[i];
				if(actualScope) fprintf(debug, "found scope symbol: %s->%s : %x\n", actualScope->object?actualScope->object->name.c_str():"NULL", name, actualScope->object);
				else fprintf(debug, "*found scope symbol: %s->%s : %x\n", scopes[i].object?scopes[i].object->name.c_str():"NULL", name, scopes[i].object);
				return r;
			}
		}

		/*if(strict)
                {
                        fprintf(debug, "failed to find %s\n", name);
                        throw "Failed to find symbol";
                }*/

		if(actualScope) actualScope->reset();
		return findSimpleSymbol(name, strict);
	}

	if(scope->type == SCOPE_CLASS)
	{
		symbol* r = scope->root->findLocalSymbol(name, false);
		if(actualScope && r) *actualScope = *scope;
		return r;
	}

	//fprintf(debug, "findSymbol(%s, %x), parent = %x\n", name, this, parent);
	for(unsigned long i=0; i < symbols.size(); i++)
	{
		if(!symbols[i]->name.compare(name))
		{
			if(actualScope) *actualScope = *scope;
			return symbols[i];
		}
	}

	if(scope)
	{
		if(scope->root == this || scope->type == SCOPE_LOCAL) return NULL;
	}

	if(parent == NULL)
	{
		if(proj)
		{
			fprintf(debug, "searching project tree\n");
			symbol* r;
			symbol* root = getRoot();
			for(unsigned long i=0; i < proj->symbols.size(); i++)
			{
				if(proj->symbols[i] == root) continue;
				r = proj->symbols[i]->findSimpleSymbol(name, false);
				if(r)
				{
					return r;
				}
			}
		}

		if(strict)
		{
			fprintf(debug, "failed to find %s\n", name);
			throw "Failed to find symbol";
		}
		return NULL;
	}

	if(type == FUNCTION)
	{
		for(unsigned long i=0; i < arguments.size(); i++)
		{
			if(!arguments[i]->name.compare(name))
			{
				if(actualScope)
				{
					actualScope->type = SCOPE_LOCAL;
					actualScope->object = NULL;
					actualScope->root = this;
				}
				return arguments[i];
			}
		}
	}
	//fprintf(debug, "%x -> %x for %s\n", this, parent, name);
	if(parent->parent == this)
	{
		fprintf(debug, "%x -> %x for %s\n", this, parent, name);
		throw "loop";
	}

	return parent->findSymbol(name, strict, scope, actualScope);
}

STRINGS* symbol::findDefine(string &name)
{
        if(!this) return NULL;

	//fprintf(debug, "searching for define %s, %d\n", name.c_str(), defines.size());
	if(defines.count(name) != 0)
	{
		fprintf(debug, "found definition! %s\n", name.c_str());
		return &defines[name];
	}

        if(parent == NULL)
        {
                return NULL;
        }

        if(parent->parent == this)
        {
                fprintf(debug, "%x -> %x for %s\n", this, parent, name.c_str());
                throw "loop";
        }

        return parent->findDefine(name);
}

symbol* symbol::findFunction(const char* name, const char* returnType, std::vector<const char*> &args, bool recursive, long depth, bool fuzzy, bool scanImports)
{
	string dt;
	for(unsigned long i=0; i < templates.size(); i++)
	{
		if(templates[i]->arguments.size() != args.size()) continue;
		bool found = true;
		for(unsigned long x=0; x < templates[i]->arguments.size(); x++)
		{
			if(strcmp(args[x], templates[i]->arguments[x]->getDatatype().c_str())) found = false;
		}
		if(found) return templates[i];
	}

	long dt_pointer;
	/*fprintf(debug, "looking for '%s'(", name);
	for(unsigned int x=0; x < args.size(); x++)
	{
		fprintf(debug, " %s", args[x]);
	}
	fprintf(debug, ")\n");*/
	for(unsigned long i=0; i < symbols.size(); i++)
	{
		//if(symbols[i]->type != FUNCTION) continue;
		if(!symbols[i]->name.compare(name))
		{
			unsigned long arg_start = 0;
			if(symbols[i]->parent && symbols[i]->parent->type == STRUCT)
			{
				arg_start = 1;
			}
			/*fprintf(debug, "checking '%s'", name);
			for(unsigned int x=0; x < symbols[i]->arguments.size(); x++)
			{
				 fprintf(debug, " %s", symbols[i]->arguments[x]->datatype?symbols[i]->arguments[x]->datatype->name.c_str():"NULL");
			}
			fprintf(debug, "\n");*/

			if(!returnType || !symbols[i]->datatype->name.compare(returnType))
			{
				if(symbols[i]->arguments.size() == args.size() + arg_start)
				{
					bool success = true;
					int y=arg_start;
					for(unsigned int x=0; x < args.size(); x++,y++)
					{
						//dt = symbols[i]->arguments[y]->getDatatype(true);
						//dt_pointer = symbols[i]->arguments[y]->pointer;
						//symbols[i]->arguments[y]->getDatatype(dt, symbols[i]->arguments[y]->datatype->name.c_str(), dt_pointer);
						dt = symbols[i]->arguments[y]->getDatatype(dt_pointer, false);
						/*if(symbols[i]->arguments[y]->datatype && symbols[i]->arguments[y]->datatype->type == STRUCT)
						{
							dt += "*";
						}*/
						if(fuzzy)
						{
							const char* base = "";
							//if( !strcmp(args[x], "void*") && symbols[i]->arguments[y]->pointer == 1 /*- (symbols[i]->arguments[y]->type == STRUCT?1:0)*/ ) continue;
							if( symbols[i]->arguments[y]->datatype)
							{
								base = symbols[i]->arguments[y]->datatype->name.c_str();
							}
							else
							{
								if(symbols[i]->arguments[y]->statement.size()) base = symbols[i]->arguments[y]->statement[0];
							}
							//fprintf(debug, "fuzzy comparing: %d %d '%s' '%s'\n", dt_pointer, pointerCount(args[x]), args[x], dt.c_str());
							if( !strcmp(base, "macro") || (!strcmp(base, "void") && dt_pointer == pointerCount(args[x])) )
							{
								fprintf(debug, "fuzzy match!\n");
								continue;
							}
						}
						fprintf(debug, "%s comparing %d %d %s and %s for %s - %s %x\n", fuzzy?"fuzzy":"", dt_pointer, pointerCount(args[x]), dt.c_str(), args[x], name, symbols[i]->arguments[y]->name.c_str(), this);
						if(strcmp(args[x], dt.c_str()))
						{
							//if(dt_pointer == pointerCount(args[x])) continue;
							//fprintf(debug, "%s comparing %d %d %s and %s for %s - %s %x\n", fuzzy?"fuzzy":"", dt_pointer, pointerCount(args[x]), dt.c_str(), args[x], name, symbols[i]->arguments[y]->name.c_str(), this);
							if(*args[x] && *dt.c_str()) success = false;
							else fprintf(debug, "match!\n");
							break;
						}
						else
						{
							fprintf(debug, "partial match!\n");
						}
					}
					if(success)
					{
						string base;
						if(!symbols[i]->isTemplate) return symbols[i];
						fprintf(debug, "found template %s", name);
						for(unsigned int u=0; u < args.size(); u++)
						{
							fprintf(debug, " %s", args[u]);
						}
						fprintf(debug, "\n");
						symbol* f = symbols[i]->findFunction(name, returnType, args, false);
						if(f) return f;

						symbol* n = symbols[i]->clone(symbols[i]->parent);
						for(unsigned int x=0; x < n->arguments.size(); x++)
						{
							if(!n->arguments[x]->datatype || !n->arguments[x]->datatype->name.compare("macro"))
							{
								datatypeBase(args[x], &base);
								n->arguments[x]->datatype = symbols[i]->findSymbol(base.c_str());
								n->arguments[x]->pointer = (char)pointerCount(args[x]);
								fprintf(debug, "setting macro datatype %s, %x\n", base.c_str(), n->arguments[x]->datatype);
								if(n->datatype && !n->datatype->name.compare("macro"))
								{
									n->datatype = n->arguments[x]->datatype;
									n->pointer = n->arguments[x]->pointer;
								}
							}
						}

fprintf(debug, "pushing new macro body %x %s(", n, n->name.c_str());
                                                for(unsigned int j=0; j < n->arguments.size(); j++)
                                                {
                                                        if(!j)
                                                        {
                                                                fprintf(debug, "%s", n->arguments[j]->datatype->name.c_str());
                                                        }
                                                        else
                                                        {
                                                                fprintf(debug, ", %s", n->arguments[j]->datatype->name.c_str());
                                                        }
                                                }
fprintf(debug, ")\n");

						n->isTemplate = false;
						if(symbols[i]->parent /*&& symbols[i]->parent->parent*/) n->setIndex(); //n->index = symbols[i]->parent->functionCount++;
						//n->resolveInheritance(statement);
						n->resolveStatements();
						symbols[i]->templates.push_back(n);
fprintf(debug, "pushing new macro body %x %s(", n, n->name.c_str());
						for(unsigned int j=0; j < n->arguments.size(); j++)
						{
							if(!j)
							{
								fprintf(debug, "%s", n->arguments[j]->datatype->name.c_str());
							}
							else
							{
								fprintf(debug, ", %s", n->arguments[j]->datatype->name.c_str());
							}
						}
fprintf(debug, ")\n");
						return symbols[i]->templates[symbols[i]->templates.size()-1];
					}
				}
				else
				{
					/*fprintf(debug, "incorrect func args for %s: %d - %d\n", name, symbols[i]->arguments.size(), args.size());
					for(unsigned int x=0; x < symbols[i]->arguments.size(); x++)
					{
						dt = symbols[i]->arguments[x]->getDatatype(dt_pointer, false);
						fprintf(debug, "-%s-\n", dt.c_str());
					}*/
				}
			}
		}
	}
	if(parent == NULL || !recursive)
	{
		if(proj && scanImports)
                {
                        //fprintf(debug, "searching project tree for function\n");
                        symbol* r;
                        symbol* root = getRoot();
                        for(unsigned long i=0; i < proj->symbols.size(); i++)
                        {
                                if(proj->symbols[i] == root) continue;
                                r = proj->symbols[i]->findFunction(name, returnType, args, recursive, depth, fuzzy, false);
                                if(r)
                                {
                                        return r;
                                }
                        }
                }

		if(!fuzzy)
		{
			return findFunction(name, returnType, args, recursive, depth, true);
		}
		return NULL;
	}
	symbol* r = parent->findFunction(name, returnType, args, recursive, depth+1, fuzzy);

	if(!r && depth == 0)
	{
		fprintf(debug, "doing fuzzy search for func '%s'\n", name);
		return parent->findFunction(name, returnType, args, recursive, 1, true);
	}
	else if(depth == 0)
	{
		fprintf(debug, "giving up search for '%s'\n", name);
	}

	return r;
}

symbol* symbol::findFunction(const char* name, const char* returnType, bool recursive, bool scanImports)
{
	std::vector<const char*> args;
	return findFunction(name, returnType, args, recursive, scanImports);
}

symbol* symbol::findFunction(const char* name, const char* returnType, const char* arg1, bool recursive, bool scanImports)
{
	std::vector<const char*> args;
	args.push_back(arg1);
	return findFunction(name, returnType, args, recursive, scanImports);
}

symbol* symbol::findFunction(const char* name, const char* returnType, const char* arg1, const char* arg2, bool recursive, bool scanImports)
{
	std::vector<const char*> args;
	args.push_back(arg1);
	args.push_back(arg2);
	return findFunction(name, returnType, args, recursive, scanImports);
}

symbol* symbol::findFunction(const char* name, const char* returnType, const char* arg1, const char* arg2, const char* arg3, bool recursive, bool scanImports)
{
	std::vector<const char*> args;
	args.push_back(arg1);
	args.push_back(arg2);
	args.push_back(arg3);
	return findFunction(name, returnType, args, recursive, scanImports);
}

symbol* symbol::findFunction(const char* name, const char* returnType, const char* arg1, const char* arg2, const char* arg3, const char* arg4, bool recursive, bool scanImports)
{
	std::vector<const char*> args;
	args.push_back(arg1);
	args.push_back(arg2);
	args.push_back(arg3);
	args.push_back(arg4);
	return findFunction(name, returnType, args, recursive, scanImports);
}

void symbol::setParents()
{
	for(unsigned long i=0; i < arguments.size(); i++)
	{
		arguments[i]->parent = this;
		arguments[i]->depth = this->depth+1;
		arguments[i]->setParents();
	}

	for(unsigned long i=0; i < symbols.size(); i++)
	{
		symbols[i]->parent = this;
		symbols[i]->depth = this->depth+1;
		symbols[i]->setParents();
	}
}

std::vector<symbol*> symbol::parseParameters(STRINGS &tokens, unsigned long &i, unsigned long end)
{
	std::vector<symbol*> result;
	if(!end) end = (unsigned long)tokens.size();
	if (!tokens.compare(i, "("))
	{
		STRINGS paramTokens = parseNextSymbol(i, tokens);
		fprintf(debug, "parsing parameter tokens %s: %d: '%s'\n", name.c_str(), paramTokens.size(), paramTokens.c_str());
		for(unsigned int x=0; x < paramTokens.size(); x++)
		{
			fprintf(debug, "'%s' ", paramTokens[x]);
		}
		fprintf(debug, "\n");
		for(unsigned long y=1; y < paramTokens.size()-1; y++)
		{
			symbol *p = new symbol(parseStatement(paramTokens, y));
			p->error = error;
			p->debug = debug;
			result.push_back(p);
		}
		//i++;
	}
	for(unsigned long x=0; x < result.size(); x++)
	{
		fprintf(debug, "param: %d: ", result[x]->statement.size());
		for(unsigned long y=0; y < result[x]->statement.size(); y++) fprintf(debug, "%s ", result[x]->statement[y]);
		fprintf(debug, "\n");
	}
	//exit(0);
	return result;
}

void symbol::getStatementEnd(STRINGS &tokens, unsigned long i, unsigned long &end, bool &hasBody, bool ignoreCommas)
{
	hasBody = false;
	long open_paren=0;
	long open_brackets = 0;
	for(end=i; end < tokens.size(); end++)
	{
		if(!open_paren && !open_brackets)
		{
			if(!tokens.compare(end, ";"))
			{
				//end--;
				break;
			}
			if(!ignoreCommas && !tokens.compare(end, ","))
			{
				//end--;
				break;
			}
		}
		if(!tokens.compare(end, "{"))
		{
			if(!open_paren) hasBody = true;
			open_brackets++;
			continue;
		}
		else if(!tokens.compare(end, "("))
		{
			open_paren++;
		}
		else if(!tokens.compare(end, ")"))
		{
			if(!open_paren) break;
			open_paren--;
		}
		else if(!tokens.compare(end, "}"))
		{
			if(!open_brackets && !open_paren)
			{
				break;
			}
			open_brackets--;
		}
	}
}

symbol symbol::parseStatement(STRINGS &tokens, unsigned long &i)
{
	bool hasBody;
	unsigned long end;
	symbol s;
	s.parent = this;
	s.type = STATEMENT;

	getStatementEnd(tokens, i, end, hasBody, false);

	for(unsigned long x=i; x < end; x++)
	{
		s.statement.push_back(tokens[x], tokens.length(x), tokens(x));
	}

	i = end;
	return s;
}

/*RESULT symbol::resolveExpression(string &buffer, symbol* p, long depth)
{
unsigned long i = 0;
return resolveExpression(i, buffer, p, depth);
}*/

symbol* symbol::closestStruct()
{
	if(parent == NULL)
	{
		return NULL;
	}
	else if(parent->type == STRUCT) return parent;
	if(parent->parent == this)
	{
		throw "loop";
	}
	return parent->closestStruct();
}

symbol* symbol::getRoot()
{
	if(parent == NULL) return this;
	return parent->getRoot();
}

void symbol::getOperators(OPERATORS &operators)
{
	for(unsigned int i=0; i < symbols.size(); i++)
	{
		if(symbols[i]->type == PRECEDENCE)
		{
			switch(rtl)
			{
			case 1: operators[symbols[i]->name][1 + (symbols[i]->pointer == 1?0:2)] = symbols[i]; break;
			case 2: operators[symbols[i]->name][4 + (symbols[i]->pointer == 1?0:2)] = symbols[i]; break;
			case 0:
			default:
				operators[symbols[i]->name][0 + (symbols[i]->pointer == 1?0:2)] = symbols[i];
			}
		}
		else
		{
			symbols[i]->getOperators(operators);
		}
	}

}

STRINGS symbol::parseNextSymbol(unsigned long &i, STRINGS &statement)
{
	STRINGS s;
	if(i >= statement.size()) return s;
	if(!statement.compare(i, "("))
	{
		long open = 1;
		//s.push_back("(");
		while(i < statement.size())
		{
			s.push_back(statement[i], statement.length(i), statement(i));
			if(!statement.compare(i, "("))
			{
				open++;
			}
			else if(!statement.compare(i, ")"))
			{
				if(--open <= 1)
				{
					i++;
					break;
				}
			}
			else if(!open && !statement.compare(i, ","))
			{
				i++;
				break;
			}
			i++;
		}
	}
	else
	{
		s.push_back(statement[i], statement.length(i), statement(i));
		i++;
	}
	return s;
}

RESULTSET symbol::resolveStatement()
{
	unsigned long x = 0;
	RESULTSET r(this, statement);
	return r;
}

RESULTSET symbol::resolveStatements(bool processBody)
{
	RESULTSET r;
	if(type == EXTERN || isTemplate)
	{
		return r;
	}

	string buffer;
	unsigned long x=0;
	//const char* first;

	if(type == CONTROL)
	{
		for(unsigned long i=0; i < arguments.size(); i++)
		{
			fprintf(debug, "control statement parsing (%s)\n", arguments[i]->statement.c_str());
			//arguments[i]->resolveStatements(false);
			//arguments[i]->parse(arguments[i]->statement, x);
			r.parse(this, arguments[i]->statement);
			arguments[i]->name = r.last().resolved;
			fprintf(debug, "resolved to: %s\n", arguments[i]->name.c_str());
		}
	}
	else
	{
		for(unsigned long i=0; i < arguments.size(); i++)
		{
			if(arguments[i]->type == STATEMENT)
			{
				x = 0;
				//RESULT tmp;
				buffer = "";
				//r = arguments[i]->resolveExpression(buffer);
				//tmp.parse(this, arguments[i]->statement);
				//arguments[i]->parse(arguments[i]->statement, x);
			}
			/*else if(arguments[i]->type == CONTROL)
			{
			x = 0;
			buffer = "";
			arguments[i]->parse(arguments[i]->statement, x);
			fprintf(debug, "parsing arguments %d\n", arguments[i]->statement.size());
			}*/
		}
	}

	if(!processBody) return r;

	//fprintf(debug, "looping through %d\n", symbols.size());
	for(unsigned long i=0; i < symbols.size(); i++)
	{
		if(symbols[i]->type == CONTROL)
		{
			r = symbols[i]->resolveStatements();
			//r.parse(this, symbols[i]->statement);
			//symbols[i]->name = r.last().resolved;
			fprintf(debug, "control statement! %d\n", symbols[i]->statement.size());
			continue;
		}
		if(symbols[i]->type != STATEMENT)
		{
			r = symbols[i]->resolveStatements();
			//r.parse(&symbols[i], symbols[i]->statement);
			continue;
		}
		x = 0;

		if(!strcmp("return", symbols[i]->statement[0]))
		{
			unsigned long x=1;
			r.parse(symbols[i], symbols[i]->statement, x);
			//r = symbols[i]->resolveExpression(++x, buffer);
			symbols[i]->name = "return ";
			symbols[i]->name += r.last().resolved;
		}
		else
		{
			r.parse(this/*&symbols[i]*/, symbols[i]->statement);
			if(symbols[i]->type == STATEMENT) symbols[i]->name = r.last().resolved;
		}
	}
	return r;
}

symbol* symbol::createChild()
{
	symbol *sym = new symbol();
	symbols.push_back(sym);
	sym->parent = this;
	sym->debug = debug;
	sym->error = error;
	//sym->init();
	//fprintf(debug, "allocing child @ %x\n", sym);
	return sym;
}

void symbol::createChild(symbol &sym)
{
	addChild(sym);
}

string symbol::render(unsigned char showref, symbol* classBase, symbol* root, sscope* scope)
{
	string buffer;
	render(buffer, showref, classBase, root);
	return buffer;
}

unsigned long symbol::incRefCount(symbol* root)
{
	if(!root) return 0;
	referenced++;
	if(datatype && datatype != this) root->references[datatype]++;
	return root->references[this]++;
}

string& symbol::render(sscope* scope, string& s, symbol* root)
{
	if(scope && scope->object)
	{
		if(scope->object->pointer > 1)
		{
			for(int i=1; i < scope->object->pointer; i++) s += "*";
		}
		s += scope->object->name.c_str();
		if(scope->object->pointer || scope->object->reference)
		{
			s += "->";
		}
		else
		{
			s += ".";
		}
	}
	render(s, NULL);
	return s;
}

string& symbol::renderMember(string& s, symbol* member)
{
	if(pointer > 1)
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
        if(member) member->render(s, NULL);
        return s;
}

string& symbol::render(string &buffer, unsigned char showref, symbol* classBase, symbol* root, sscope* scope)
{
	if(this == NULL) return buffer;
	if(root)
	{
		root->references[this]++;
	}
	if(showref)
	{
		for(char i=0; i < pointer; i++)
		{
			buffer += "*";
		}
		if(showref == REF_FULL && reference) buffer += "*";
	}

	if(type == FUNCTION)
	{
		if(external)
		{
			buffer += name;
			return buffer;
		}

		if(parent != NULL && parent->parent == NULL && name == "main")
		{
			buffer += "main";
			return buffer;
		}

		for(unsigned long i=0; i < name.size(); i++)
		{
			if(isalnum(name[i]))
			{
				buffer += name[i];
			}
			else
			{
				buffer += ultoa(name[i], 26);
			}
		}
		buffer += '$';
		//buffer += ultoa((long)this, 64);
		if(parent && parent->type == STRUCT)
		{
			buffer += parent->name.c_str();
			buffer += '$';
		}
		buffer += ultoa(depth, 10);
		if(index)
		{
			buffer += "_";
			buffer += ultoa(index, 10);
		}
	}
	else if(type == STRUCT)
	{
		buffer += "struct " + name;// + "*";
	}
	else if(type == VARIABLE)
	{
		if(name.size() == 0)
		{
			buffer += "NULL";
		}
		else
		{
			if(classBase)
			{
				classBase->render(buffer);
				buffer += "->";
			}
			else if(parent && parent->type == STRUCT)
			{
				//buffer += "this->";
			}
			else
			{
				/*buffer += "(";
				buffer += ultoa((long)this, 16);
				buffer += ")";*/
			}
			buffer += name;
		}
	}
	else
	{
		if(name.size() == 0 && 0)
		{
			buffer += "NULL";
		}
		else buffer += name;
	}
	return buffer;
}

void symbol::printTree(FILE* fd, int depth)
{
	if(!fd) fd = error;
	for(int i=0; i < depth; i++) fprintf(fd, "\t");
	fprintf(fd, "%d %x ", type, datatype/*getDatatype().c_str()*/);
	for(int i=0; i < pointer; i++) fprintf(fd, "*");
	fprintf(fd, "%s %x", name.c_str(), this);
	if(type == FUNCTION)
	{
		fprintf(fd, "(");
		for(unsigned int i=0; i < arguments.size(); i++)
		{
			if(i) fprintf(fd, ", %s %s", arguments[i]->getDatatype().c_str(), arguments[i]->name.c_str());
			else fprintf(fd, "%s %s", arguments[i]->getDatatype().c_str(), arguments[i]->name.c_str());
		}
		fprintf(fd, ")");
	}
	else
	{
		for (std::map < string, STRINGS >::iterator pos = defines.begin(); pos != defines.end(); ++pos)
		{
			{
				fprintf(fd, "\n");
				for(int i=0; i < depth; i++) fprintf(fd, "\t");
			}
			fprintf(fd, "define %s ", pos->first.c_str());
			for(unsigned long t=0; t < pos->second.size(); t++)
			{
				fprintf(fd, " %s", pos->second[t]);
			}
		}
	}
	if(1)
	{
		fprintf(fd, "\tScopes:");
		for(unsigned int i=0; i < scopes.size(); i++)
		{
			if(scopes[i].object) fprintf(fd, " %x(%s)", scopes[i].root, scopes[i].object->name?scopes[i].object->name.c_str():"NULL");
			else fprintf(fd, " %x", scopes[i].root);
		}
	}
	fprintf(fd, "\n");
	depth++;

	for(unsigned int i=0; i < symbols.size(); i++)
	{
		symbols[i]->printTree(fd, depth);
	}
}

inline void symbol::addChild(symbol &sym)
{
	if(sym.type == VARIABLE && parent)
	{
		*this = sym;
	}
	else
	{
		symbol* n = new symbol(sym, this);
		//n->error = error;
		symbols.push_back(n);
	}
}

void symbol::setScope()
{
	/*if(!parent) // push global scope
        {
                sscope s;
                s.type = SCOPE_GLOBAL;
                s.root = this;
		s.object = this;
                //scopes.push_back(s);
		pushScope(&s);
        }*/

	for(unsigned long i=0; i < symbols.size(); i++)
	{
		symbols[i]->setScope();
	}

	for(unsigned long i=0; i < scopes.size(); i++)
	{
		pushScope(&scopes[i]);
	}

	if(parent && parent->type == STRUCT && type == FUNCTION)
	{
		sscope s;
		s.type = SCOPE_CLASS;
		s.root = parent;
		if(arguments.size())
		{
			s.object = arguments[0];
		}
		scopes.push_back(s);
		pushScope(&s);
	}
}

void symbol::pushScope(sscope *s)
{
	for(unsigned long i=0; i < symbols.size(); i++)
	{
		symbols[i]->scopes.push_back(*s);
		symbols[i]->pushScope(s);
	}
}

void symbol::parse(STRINGS & tokens, unsigned long &i)
{
	if(parent) depth = parent->depth + 1;
	//fprintf(debug, "parse(%d) %s %x  parent: %x\n", i, tokens[i], this, this->parent);
	string buffer;
	bool hasBody = false, isFunction = false;
	unsigned long open_brackets = 0;
	unsigned long open_paren = 0;
	unsigned long end = 0;
	if(i >= tokens.size()) return;

	if(!tokens.compare(i, "define"))
	{
		++i;
		string defname = tokens[i++];

		while(i < tokens.size())
		{
			if(!tokens.compare(i, ";"))
			{
				i++;
				break;
			}
			parent->defines[defname].clear();
			parent->defines[defname].push_back(tokens[i], tokens.length(i), tokens(i));
			i++;
		}

		fprintf(debug, "defining: %s @%x", defname.c_str(), this);
		for(unsigned t=0; t < parent->defines[defname].size(); t++)
		{
			fprintf(debug, " %s", parent->defines[defname][t]);
		}
		fprintf(debug, "\n");
		return;
	}

	if(!tokens.compare(i, "function"))
	{
		isFunction = true;
		if(!tokens.compare(++i, "inline"))
		{
			isInline = true;
			i++;
		}
	}
	/*else if(!tokens.compare(i, "macro"))
	{
		isFunction = true;
		isMacro = true;
		hidden = true;
		i++;
	}*/
	else if(!tokens.compare(i, "operator"))
	{
		isOperator = true;
		isFunction = true;
		i++;

		if(!tokens.compare(i, "rtl"))
		{
			rtl = OP_RTL;
		}
		else if(!tokens.compare(i, "ltr"))
		{
			rtl = OP_LTR;
		}
		else if(!tokens.compare(i, "sub"))
		{
			rtl = OP_SUB;
		}
		else
		{
			throw "expected token rtl OR ltr OR sub for operator";
		}
		i++;
		if(!tokens.compare(i, "verbatim"))
		{
			verbatim = true;
			i++;
		}
	}
	else if(!tokens.compare(i, "precedence"))
	{
		type = PRECEDENCE;
		hidden = true;

		i++;

		if(!tokens.compare(i, "rtl"))
		{
			rtl = 1;
		}
		else if(!tokens.compare(i, "ltr"))
		{
			rtl = 0;
		}
		else if(!tokens.compare(i, "sub"))
		{
			rtl = 2;
		}
		else
		{
			throw "expected token rtl OR ltr OR sub for operator";
		}

		pointer = (char)atol(tokens[++i]);
		i++;

		name = ""; //"precedence$";
		while(i < tokens.size() && tokens.compare(i+1, ";"))
		{
			name += tokens[i++];
		}

		precedence = atol(tokens[i++]);
	}

	if(!tokens.compare(i, ";"))
	{
		i++;
		return;
	}
	else if (!tokens.compare(i, "{"))
	{
		if(!type) type = SCOPE;
		i++;
		while(i < tokens.size())
		{
			if (!tokens.compare(i, ";"))
			{
				i++;
				continue;
			}
			else if (!tokens.compare(i, "}"))
			{
				break;
			}
			/*symbol sym;
			sym.parent = this;
			sym.parse(tokens, i);
			addChild(sym);*/
			createChild()->parse(tokens, i);
		}

		if (tokens.compare(i++, "}"))
		{
			fprintf(debug, "got %s *%s* %s\n", tokens[i - 2], tokens[i - 1], tokens[i]);
			throw "expected } to close body";
		}
		return ;
	}
	else if (!tokens.compare(i, "struct") || !tokens.compare(i, "class"))
	{
		name = tokens[++i];
		type = STRUCT;
		if(!tokens.compare(i+1, ":"))
		{
			i++;
			do
			{
				fprintf(debug, "extending %s -> %s\n", name.c_str(), tokens[i+1]);
				extends.push_back(tokens[++i]);
			}
			while(i < tokens.size() && !tokens.compare(i, ","));
		}
		parse(tokens, ++i);
		i++;
		type = STRUCT;
		incRefCount(getRoot());
		return;
	}
	else if(!tokens.compare(i, "extern"))
	{
		/*symbol sym;
		i++;
		sym.parse(tokens, i);
		type = EXTERN;
		addChild(sym);*/
		createChild()->init(EXTERN, "")->parse(tokens, ++i);
		return;
	}
	else if (!tokens.compare(i, "if") || !tokens.compare(i, "else") || !tokens.compare(i, "while") || !tokens.compare(i, "for") || !tokens.compare(i, "foreach") || !tokens.compare(i, "do") || !tokens.compare(i, "with"))
	{
		name = tokens[i++];
		type = CONTROL;
	}
	else if(!tokens.compare(i, "import") || !tokens.compare(i, "include"))
	{
		bool include = !tokens.compare(i, "include");
		//type = IMPORT;

		arguments = parseParameters(tokens, ++i, end);
		if(!tokens.compare(i, "{"))
		{
			i++;
			while(i < tokens.size())
			{
				if (!tokens.compare(i, ";"))
				{
					i++;
					continue;
				}
				else if (!tokens.compare(i, "}"))
				{
					break;
				}

				symbol* sym = parent->createChild();
				sym->parse(tokens, i);
				sym->hidden = true;
				fprintf(debug, "Adding import %s to '%s' @ %x parent: %x\n", sym->name.c_str(), sym->datatype?sym->datatype->name.c_str():"NULL", sym->datatype, parent);
				fprintf(debug, "symbol %s @ %x %x\n", sym->datatype->name.c_str(), findSymbol(sym->datatype->name.c_str()), parent->findSymbol(sym->datatype->name.c_str()));
				//parent->addChild(*sym);
			}

			if (tokens.compare(i++, "}"))
			{
				fprintf(debug, "got %s *%s* %s\n", tokens[i - 2], tokens[i - 1], tokens[i]);
				throw "expected } to close body";
			}
		}
		else
		{
			symbol* root = getRoot();
			if(include)
			{
				loadLibrary(tokens[i], *parent, include);
			}
			else
			{
				unsigned int matches=0;
				for(unsigned int x=0; x < root->imports.size(); x++)
				{
					if(!root->imports[x].compare(tokens[i])) matches++;
				}
				if(!matches)
				{
					root->imports.push_back(tokens[i]);
					loadLibrary(tokens[i], *parent, include);
				}
			}
			i += 2;
			parent->setParents();
			//parse(tokens, i);
		}
		return;
	}
	else if(!tokens.compare(i, "pod"))
	{
		symbol* sym = parent->createChild()->init(POD, tokens[++i]);
		fprintf(debug, "creating pod %s @ %x\n", tokens[i], sym);
		i+=2;
		return;
	}

	getStatementEnd(tokens, i, end, hasBody);

	for(unsigned int x=1; i+x < tokens.size(); x++)
	{
		if(!tokens.compare(i+x, "*") || !tokens.compare(i+x, "&"))
		{
			continue;
		}
		//if(!tokens.compare(i+x+1, "(")) hasBody = true;
		break;
	}

	if(isFunction || type == STRUCT || type == CONTROL || type == IMPORT && type != STATEMENT)
	{
		if(type == CONTROL || type == IMPORT)
		{
		}
		else if(isFunction)
		{
			type = FUNCTION;
			//index = parent->functionCount++;
			setIndex();

			if(tokens.compare(i, "(")) // if not an open paren, expecting datatype
			{
				if(isOp(*tokens[i]))
				{
					datatype = NULL;
				}
				else
				{
					datatype = parent->findSymbol(tokens[i]);
				}

				if(datatype) i++; // increment only if datatype is specified
				if (!tokens.compare(i, ",") || !tokens.compare(i, ")"))
				{
					fprintf(error, "unexpected token in function declaration: %s\n", tokens[i]);
					throw "unexpected token";
					return;
				}

				while(!tokens.compare(i, "*") && tokens.compare(i+1, "("))
				{
					pointer++;
					i++;
				}

				if(!tokens.compare(i, "&"))
				{
					reference = true;
					i++;
				}

				name = tokens[i++];
			}
			else
			{
				datatype = parent->findSymbol("void");
				name = "anonymous";
			}

			fprintf(debug, "function declared: %s\n", name.c_str());

			if(isOperator)
			{
				if(rtl == OP_RTL) name += "$";
				else if(rtl == OP_SUB) name += "$$";
				//else name = "$" + name;
			}
		}
		else
		{
			//fprintf(debug, "function: %s\n", tokens[i]);
			throw "Unknown code body";
		}
		arguments = parseParameters(tokens, i, end);

		if(type == CONTROL)
		{
			fprintf(debug, "found %d control arguments\n", arguments.size());
			for(unsigned int t=0; t < arguments.size(); t++)
			{
				fprintf(debug, "arg %d: %s type: %d\n", t, arguments[t]->statement.c_str(), arguments[t]->type);
				//fprintf(debug, "resolved to: %s\n", arguments[t].name.c_str());
			}
		}
		else
		{
			fprintf(debug, "found  %d arguments\n", arguments.size());
			for(unsigned int x=0; x < arguments.size(); x++)
			{
				arguments[x]->resolveStatements(false);
				if(!arguments[x]->statement.size()) continue;
				if(arguments[x]->getDatatype() == "macro")
				{
					//fprintf(debug, "found template %s\n", name.c_str());
					isTemplate = true;
					break;
				}
			}

			if(parent && parent->type == STRUCT)
			{
				symbol* t = new symbol(VARIABLE, parent,  "this", parent);
				t->pointer = 1;
				arguments.insert(arguments.begin(), t);
			}

		}

		if (!tokens.compare(i, ";"))
		{
			isTemplate = false;
			external = true;
		}
		else
		{
			/*symbol sym;
			sym.parse(tokens, i);
			addChild(sym);*/
			createChild()->parse(tokens, i);
		}
	}
	else // statement
	{
		symbol* s = parent->findSymbol(tokens[i], false);
		if(s && s->isDatatype()) // variable declaration
		{
			type = VARIABLE;
			datatype = parent->findSymbol(tokens[i++]);
			if(datatype) datatype->incRefCount(getRoot());
			pointer = 0;
			reference = false;

			while(*tokens[i] == '*')
			{
				pointer++;
				i++;
			}
			if(*tokens[i] == '&')
			{
				reference = true;
				i++;
			}
			fprintf(debug, "adding variable '%s', datatype: %x type: %x\n", tokens[i], datatype, datatype->type);
			name = tokens[i++];

			if(i < tokens.size() && tokens.compare(i, ";"))
			{
				i--;
			}
		}
		else // statement
		{
			if(end > i)
			{
				string search;
				STRINGS* s;
				type = STATEMENT;
				for(unsigned long x=i; x <= end; x++)
				{
					search = tokens[x];
					if(!search.compare("open_x"))
					{
						type = STATEMENT;
					}
					s = findDefine(search);
					if(s)
					{
fprintf(debug, "found definition!\n");
						for(unsigned long j=0; j < s->size(); j++)
						{
							statement.push_back((*s)[j], s->length(j), (*s)(j));
						}
					}
					else
					{
						statement.push_back(tokens[x], tokens.length(x), tokens(x));
					}
				}

				fprintf(debug, "pushed statement (%d - %d): ", i, end);
				for(unsigned long x=0; x < statement.size(); x++) fprintf(debug, " %s", statement[x]);
				fprintf(debug, "\n");
				i = end +0;
			}
			else
			{
				fprintf(debug, "statement did not progress!\n");
				i++;
			}
		}
	}
}

void symbol::setIndex()
{
	symbol* root = getRoot();
	if(root && root->proj)
	{
		index = root->proj->currentIndex++;
	}
}

symbol* symbol::clone(symbol* parent)
{
	symbol* f = new symbol(*this);
	f->parent = parent;

	if(f->type == FUNCTION && f->parent)
	{
		//f->index = f->parent->functionCount++;
		f->setIndex();
	}

	for(unsigned long i=0; i < arguments.size(); i++)
	{
		f->arguments[i] = arguments[i]->clone(this);
		f->arguments[i]->parent = f;
	}

	for(unsigned long i=0; i < symbols.size(); i++)
	{
		f->symbols[i] = symbols[i]->clone(this);
		f->symbols[i]->parent = f;
	}
	return f;
}

int symbol::resolveInheritance(STRINGS &tokens)
{
	if(type == STRUCT)
	{
		if(extends.size())
		{
			fprintf(debug, "found extension '%s' for '%s'\n", extends[0].c_str(), name.c_str());
			for(unsigned int i=0; i < extends.size(); i++)
			{
				symbol* f = findSymbol(extends[i].c_str());
				if(!f)
				{
					throw "did not find extension class";
					continue;
				}

				for(unsigned int x=0; x < f->symbols.size(); x++)
				{
					symbol* n = f->symbols[x]->clone(this);
					if(n->type == FUNCTION)
					{
						n->arguments[0]->datatype = this;
					}
					symbols.push_back(n);
					//addChild(*(f->symbols[x]));
					//symbols.push_back(f->symbols[x]);
				}

				/*for(int x=0; x < symbols.size(); x++)
				{
				symbol* s = symbols[x];
				if(s->type != FUNCTION || s->arguments.size() == 0 ) continue;
				//fprintf(debug, "args: %s %d\n", s->arguments[0].datatype.c_str(), s->arguments.size());
				s->arguments[0]->datatype = findSymbol(name);
				//fprintf(debug, "first arg: %s\n", s->arguments[0].statement[0]);
				}*/
			}
			setParents();
			//resolveStatements();
		}
	}
	else if(type == FUNCTION)
	{
		unsigned long x;
		for(unsigned int i=0; i < arguments.size(); i++)
		{
			x = 0;
			fprintf(debug, "resolving argument '%s' '%s'\n", name.c_str(), arguments[i]->statement.c_str());
			arguments[i]->parse(arguments[i]->statement, x);
			//arguments[i]->resolveInheritance(tokens);
		}
	}

	if(!isTemplate)
	{
		for(unsigned int i=0; i < symbols.size(); i++)
		{
			symbols[i]->resolveInheritance(tokens);
		}
	}
	return 0;
}

void symbol::renderDefinitions(FILE* output, int depth)
{
	if(depth == 0)
	{
		fprintf(output, "// rendering function declarations\n");
		symbol* root = getRoot();
		depth++;
		for(std::map<symbol*, unsigned long>::iterator it = root->references.begin(); it != root->references.end(); it++)
		{
			it->first->renderDefinitions(output, depth);
		}
	}
	else
	{
		if(type == IMPORT || hidden == true) return;

		if(getRoot()->references[this])
		{
			if(type == FUNCTION)
			{
				if(isTemplate == true || verbatim == true)
				{
					return;
				}
				string fdatatype;
				string fname = "";
				string tname;
				datatype->render(fdatatype, REF_PTR, NULL, false);
				render(fname);
				fprintf(output, "%s %s(", fdatatype.c_str(), fname.c_str());

				for (unsigned int x = 0; x < arguments.size(); x++)
				{
					tname = "";
					fdatatype = "";
					arguments[x]->datatype->render(fdatatype, REF_PTR, NULL, false);
					arguments[x]->render(tname, REF_FULL, NULL, false);

					if (x == 0)
					{
						fprintf(output, "%s %s", fdatatype.c_str(), tname.c_str());
					}
					else
					{
						fprintf(output, ", %s %s", fdatatype.c_str(), tname.c_str());
					}
				}
				fprintf(output, ");\n");
			}
		}
	}

	/*for (unsigned int i = 0; i < symbols.size(); i++)
	{
		symbols[i]->renderDefinitions(output, depth+1);
	}*/
}

void symbol::renderDatatypes(FILE* output, int depth)
{
        if(depth == 0)
        {
		fprintf(output, "// rendering datatype declarations\n");
                symbol* root = getRoot();
                depth++;
                for(std::map<symbol*, unsigned long>::iterator it = root->references.begin(); it != root->references.end(); it++)
                {
                        it->first->renderDatatypes(output, depth);
                }
        }
        else
        {
                //if(type == IMPORT || hidden == true) return;

		if(type == STRUCT)
		{
			fprintf(output, "struct %s;\n", name.c_str());
		}
        }
}

void symbol::renderDatatypeBodies(FILE* output, int depth)
{
        if(depth == 0)
        {
		fprintf(output, "// rendering datatype bodies\n");
                symbol* root = getRoot();
                depth++;
                for(std::map<symbol*, unsigned long>::iterator it = root->references.begin(); it != root->references.end(); it++)
                {
                        it->first->renderDatatypeBodies(output, depth);
                }
        }
        else
        {
                //if(type == IMPORT || hidden == true) return;

                if(type == STRUCT)
                {
			renderCDef(output, *this,  "", 0);
                        //fprintf(output, "struct %s;\n", name.c_str());
                }
        }
}

void symbol::renderTemplateDefinitions(FILE* output, int depth)
{
	if(type == IMPORT || hidden == true) return;
	if(type == FUNCTION)
	{
		if(isTemplate == true)
		{
			for(unsigned long i=0; i < templates.size(); i++)
			{
				templates[i]->renderDefinitions(output, depth+1);
			}
			return;
		}
	}

	for (unsigned int i = 0; i < symbols.size(); i++)
	{
		symbols[i]->renderTemplateDefinitions(output, depth+1);
	}
}

void symbol::compile(const char *file, const char* sourceFile, const char* errorFile, const char* debugFile)
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
	compile(file, sourceFile, error, debug);
}

void symbol::compile(const char *file, const char* sourceFile, FILE* errorFile, FILE* debugFile)
{
	error = errorFile;
	debug = debugFile;

	if(sourceFile && *sourceFile)
	{
		output = fopen(sourceFile, "w+");
		if(!output)
		{
			output = stdout;
		}
	}

	getRoot()->imports.push_back(string(file));

	STRINGS m_tokens;
	string buf = "#include <unistd.h>\n#include <malloc.h>\n#include <string.h>\n#define bool unsigned char\n";
	file_get_contents(file, buf);
	tokenize(buf.c_str(), m_tokens);

	/*for(unsigned long i=0; i < m_tokens.size(); i++)
	{
	fprintf(debug, "%s\n", m_tokens[i]);
	}
	return;*/

	//setScope();
	unsigned long x = 0;
	while (x < m_tokens.size())
	{
		createChild()->parse(m_tokens, x);
		//fprintf(debug, "end parse\n\n");
		/*symbol s;
		s.parent = this;
		s.parse(m_tokens, x);
		addChild(s);*/
	}

	resolveInheritance(m_tokens);
	setScope();
	resolveStatements();
	fprintf(output, "#include \"include/c/std.h\"\n\n");
	printHeaders(output);
	renderC(output, *this, string(), -1);

	if(error != stderr)
	{
		fclose(error);
	}
}

void symbol::loadLibrary(const char *file, symbol &parent, bool include)
{
	//fprintf(debug, "loading %s\n", file);
	STRINGS m_tokens;
	string buf;
	string full_path = string("include/") + file + ".sx";
	string c_file = string("bd/") + file + ".c";
	boost::filesystem::path dir(c_file.c_str());
	boost::filesystem::create_directories(dir.parent_path());
	file_get_contents(full_path, buf);
	tokenize(buf.c_str(), m_tokens);

	symbol* s;

	unsigned long x = 0;

	symbol* root = parent.getRoot();
	if(!root || !root->proj)
	{
		if(!root) throw "unable to get root";
		if(!root->proj) throw "unable to get project";
		return;
	}

	if(include)
	{
		while (x < m_tokens.size())
		{
			/*symbol s;
			s.parent = &parent;
			s.parse(m_tokens, x);
			parent.addChild(s);*/

			s = parent.createChild();
			s->isImport = !include;
			s->parse(m_tokens, x);

		}
	}
	else
	{
		fprintf(error, "importing '%s' %d\n", full_path.c_str(), root->proj->files.size());
		symbol* imp = root->proj->compile(full_path.c_str(), c_file.c_str(), error, debug);
		//imp.imports = parent.getRoot()->imports;
		//imp.imports.push_back(string(file));
		//imp.compile(full_path.c_str(), c_file.c_str(), error, debug);

		/*for(unsigned long i=0; i < imp->symbols.size(); i++)
		{
			bool found = false;

			for(unsigned long y=0; y < parent.symbols.size(); y++)
			{
				if(!parent.symbols[y]->name.compare(imp->symbols[i]->name.c_str()))
				{
					found = true;
					break;
				}
			}

			if(!found)
			{
				unsigned long index;
				symbol *s = parent.createChild();
				index = s->index;
				*s = *imp->symbols[i];
				s->index = index;
				s->isImport = true;
				parent.symbols.push_back(s);
			}
		}*/
	}
	//parent.printTree();
	//exit(0);
}

void symbol::print()
{
	printHeaders(output);
	fprintf(output, "#include <unistd.h>\n#include <malloc.h>\n");
	//renderC(buf, *this, m_tokens, string(), -1);
}

void symbol::printHeaders(FILE* output)
{
	renderDatatypes(output);
	renderDatatypeBodies(output);
	renderDefinitions(output);
	renderTemplateDefinitions(output);
}


