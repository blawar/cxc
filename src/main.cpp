#ifdef WINDOWS
#include <conio.h>
#endif
#include <stdio.h>
#include <sx/libsx.h>
#include <sx/strings.h>
#include "project.h"
#include "symbol.h"

void renderC(FILE* output, std::vector < symbol* > &symbols, string prefix = "", int depth=0);
void renderC(FILE* output, symbol & sym, string prefix = "", int depth=0);

struct block
{
	void parse(STRINGS & tokens, unsigned long &i)
	{
	}
};


void renderRaw(FILE* output, symbol &sym)
{
	for(unsigned int i=0; i < sym.symbols.size(); i++)
	{
		if(sym.symbols[i]->type == STATEMENT)
		{
			for(unsigned int x = 0; x < sym.symbols[i]->statement.size(); x++)
			{
				fprintf(output, "%s ", sym.symbols[i]->statement[x]);
			}
			fprintf(output, "\n");
		}
		else if(sym.symbols[i]->type == SCOPE)
		{
			renderRaw(output, *sym.symbols[i]);
		}
		else
		{
			fprintf(output, "unknown %s\n", sym.symbols[i]->type);
		}
	}
}

void renderCDef(FILE* output, symbol & sym,  string prefix, int depth)
{
	string tabs;
        for(int x=0; x < depth; x++)
        {
                tabs += "\t";
        }

	if (sym.type == STRUCT)
        {
                fprintf(output, "%sstruct %s", tabs.c_str(), sym.name.c_str());
                fprintf(output, "\n%s{\n", tabs.c_str());
                for (unsigned int i = 0; i < sym.symbols.size(); i++)
                {
                        if(sym.symbols[i]->type != VARIABLE)
                        {
                                continue;
                        }
                        renderC(output, *sym.symbols[i], string(), depth+1);
                }
                fprintf(output, "%s};\n\n", tabs.c_str());
        }
}

void renderC(FILE* output, symbol & sym,  string prefix, int depth)
{
	if(sym.isImport) return;
	if(!output) output = stdout;
	string tabs;
	//if (sym.type == SCOPE || sym.type == STRUCT) depth--;
	for(int x=0; x < depth; x++)
	{
		tabs += "\t";
	}

	if (sym.type == STRUCT)
	{
		/*fprintf(output, "%sstruct %s", tabs.c_str(), sym.name.c_str());
		fprintf(output, "\n%s{\n", tabs.c_str());
		for (unsigned int i = 0; i < sym.symbols.size(); i++)
		{
			if(sym.symbols[i]->type != VARIABLE)
			{
				continue;
			}
			renderC(output, *sym.symbols[i], string(), depth+1);
		}
		fprintf(output, "%s};\n\n", tabs.c_str());*/
		//renderC(output, sym.symbols, tokens, sym.name, depth);

		for (unsigned int i = 0; i < sym.symbols.size(); i++)
		{
			if(sym.symbols[i]->type == VARIABLE)
			{
				continue;
			}
			if(!sym.symbols[i]->referenced)
			{
				fprintf(output, "// unused\n");
			}
			renderC(output, *sym.symbols[i], sym.name, depth);
		}
	}
	else if (sym.type == VARIABLE)
	{
		string datatype, name;
		sym.render(name);
		sym.datatype->render(datatype);
		fprintf(output, "%s%s %s;\n", tabs.c_str(), datatype.c_str(), name.c_str());
		//fprintf(output, "%shas %d children;\n", tabs.c_str(), sym.symbols.size());
	}
	else if (sym.type == SCOPE)
	{
		fprintf(output, "%s{\n", tabs.c_str());
		renderC(output, sym.symbols, prefix, depth+1);
		fprintf(output, "%s}\n\n", tabs.c_str());

	}
	else if (sym.type == FUNCTION)
	{
		if(sym.isTemplate == true)
		{
			for(unsigned long i=0; i < sym.templates.size(); i++)
			{
				renderC(output, *sym.templates[i], sym.name, depth);
			}
			return;
		}
		if(!sym.referenced)
		{
			fprintf(output, "// unused\n");
		}
		//fprintf(output, "%s", tabs.c_str());
		if(sym.external == true) return;
		fprintf(output, "%s", tabs.c_str());
		string datatype;
		string name = "";
		sym.datatype->render(datatype, REF_FULL);
		sym.render(name);
		if(0 && prefix.size())
		{
			fprintf(output, "%s %s(struct %s* this", datatype.c_str(), name.c_str(), prefix.c_str());
			for (unsigned int x = 0; x < sym.arguments.size(); x++)
			{
				fprintf(output, ", %s %s", sym.arguments[x]->datatype->name.c_str(), sym.arguments[x]->name.c_str());
			}
		}
		else
		{
			fprintf(output, "%s %s(", datatype.c_str(), name.c_str());
			for (unsigned int x = 0; x < sym.arguments.size(); x++)
			{
				datatype = "";
				name = "";
				sym.arguments[x]->datatype->render(datatype, REF_FULL);
				sym.arguments[x]->render(name, REF_FULL);
				if (x == 0)
				{
					fprintf(output, "%s %s", datatype.c_str(), name.c_str());
				}
				else
				{
					fprintf(output, ", %s %s", datatype.c_str(), name.c_str());
				}
			}
		}
		fprintf(output, ")\n");
		renderC(output, sym.symbols, prefix, depth);
	}
	else if (sym.type == STATEMENT)
	{
		/*fprintf(output, tabs.c_str());
		for(int x=0; x < sym.statement.size(); x++) fprintf(output, "%s ", sym.statement[x]);
		fprintf(output, ";\n");*/

		fprintf(output, "%s%s;\n", tabs.c_str(), sym.name.c_str());
		/*for (int i = 0; i < sym.symbols.size(); i++)
		{
		renderC(output, sym.symbols[i], tokens, string(), depth);
		}*/
	}
	else if (sym.type == POD)
	{
	}
	else if(sym.type == CONTROL)
	{
		fprintf(output, "%s", tabs.c_str());
		if(sym.arguments.size() == 0)
		{
			fprintf(output, "%s\n", sym.name.c_str());
		}
		else
		{
			string datatype;
			fprintf(output, "%s(", sym.name.c_str());
			for (unsigned int x = 0; x < sym.arguments.size(); x++)
			{
				datatype = "";
				//sym.arguments[x]->datatype->render(datatype);
				if (x == 0)
				{
					fprintf(output, "%s", sym.arguments[x]->name.c_str());
				}
				else
				{
					fprintf(output, ", %s", sym.arguments[x]->name.c_str());
				}
			}
			fprintf(output, ")\n");
		}
		renderC(output, sym.symbols, prefix, depth);
	}
	else if(sym.type == EXTERN)
	{
		renderRaw(output, sym);
	}
	else if(sym.type == IMPORT)
	{
	}
	else
	{
		//fprintf(output, "Unknown %d\n", sym.type);
		renderC(output, sym.symbols, string(), depth+1);
	}
}

void renderC(FILE* output, std::vector < symbol* > &symbols, string prefix, int depth)
{
	if (prefix.size())
	{
		for (unsigned int i = 0; i < symbols.size(); i++)
		{
			//if (symbols[i]->type == FUNCTION)
			renderC(output, *symbols[i], prefix, depth);
		}
	}
	else
	{
		for (unsigned int i = 0; i < symbols.size(); i++)
		{
			renderC(output, *symbols[i], prefix, depth);
		}
	}
}

void showUsage()
{
	exit(0);
}

FILE* openFile(const char* fname, const char* perms)
{
	if(!fname) return NULL;
	if(!strcmp(fname, "stdout")) return stdout;
	if(!strcmp(fname, "-")) return stdout;
	if(!strcmp(fname, "stderr")) return stderr;
	return fopen(fname, perms);
}

int main(int argc, char *argv[])
{
	project proj;
	char* tree = NULL;
	char* err = NULL;
	char* src = NULL;
	char* dbg = NULL;
	char* sourceFile = NULL;
	bool bShowMethods = false;

	for (int i = 1; i < argc; i++)
	{
		if (*argv[i] == '-')
		{
			bool found = false;
			for (int c = 1; argv[i][c] != 0 && !found; c++)
			{
				switch (argv[i][c])
				{
					case 'm':
						bShowMethods = true;
						break;
					case 't':
						if(i == argc-1) showUsage();
						tree = argv[++i];
						found = true;
						break;
					case 'o':
						if(i == argc-1) showUsage();
						src = argv[++i];
						found = true;
						break;
					case 'e':
						if(i == argc-1) showUsage();
						err = argv[++i];
						found = true;
						break;
					case 'd':
                                                if(i == argc-1) showUsage();
                                                dbg = argv[++i];
						found = true;
                                                break;
				}
			}
		}
		else
		{
			sourceFile = argv[i];
		}
	}
	try
	{
		try
		{
			proj.openFiles(err, dbg);
			proj.compile(sourceFile, src);
			//s.print();
			//return 0;
		}
		catch(const char *blah)
		{
			//s.printTree();
			fprintf(stderr, "%s\n", blah);
		}

		FILE* treef = openFile(tree, "w+");
		if(treef)
		{
			proj.printTree(treef);
			fclose(treef);
		}
	}
	catch(.../*const char *blah*/)
	{
		//fprintf(output, "%s\n", blah);
		//s.printTree();
	}
#ifdef WINDOWS
	_getch();
#endif
	return 0;
}
