#include "result.h"
#include "symbol.h"

#define MAX_FUNCTION_PARAMS 8

RESULTSET::RESULTSET()
{
}

RESULTSET::RESULTSET(symbol* parent, STRINGS &tokens)
{
}

RESULTSET::~RESULTSET()
{
}

void RESULTSET::reset()
{
}

RESULTSET* RESULTSET::parse(symbol* parent, STRINGS &tokens, unsigned long start)
{
	reset();
	for(unsigned long i=start; i < tokens.size(); i++)
	{
		//fprintf(parent->debug, "parsing result set @%d : %s\n", i, tokens[i]);
		RESULT r;
		r.parse(parent, tokens, i);
		results.push_back(r);
	}
	return this;
}

RESULTSET* RESULTSET::parse(symbol* parent, const char* buffer)
{
	reset();
	tokenize(buffer, statement);
	return this;
}

RESULT& RESULTSET::first()
{
	return get(0);
}

RESULT& RESULTSET::last()
{
	if(!size())
	{
		throw "resultset out of bounds";
	}
	return get(size()-1);
}

RESULT& RESULTSET::get(unsigned long i)
{
	if(i >= size()) throw "resultset out of bounds";
	return results[i];
}

unsigned long RESULTSET::size()
{
	return results.size();
}


RESULT::RESULT()
{
	reset();
}

RESULT::RESULT(symbol* parent, STRINGS &tokens)
{
	reset();
	parse(parent, tokens);
}

RESULT::~RESULT()
{
}

void RESULT::reset()
{
	parent = NULL;
	isConst = false;
	isOperator = false;
	isPointer = false;
	isNull = false;
	val = "";
	name = "";
	l = 0;
	type = 0;
	pointer = 0;
	reference = 0;
}

/*symbol* RESULT::sym(symbol* s)
{
if(symb)
{
delete symb;
}
if(s) symb = new symbol(*s);
else symb = NULL;
}*/

RESULT* RESULT::parse(symbol* parent, const char* buffer)
{
	tokenize(buffer, statement);
	return parse(parent, statement);
}

RESULT* RESULT::parse(symbol* parent, STRINGS &tokens)
{
	unsigned long i=0;
	return parse(parent, tokens, i);
}

RESULT* RESULT::parse(symbol* parent, STRINGS &tokens, unsigned long &i)
{
	symbol* root = parent->getRoot();
	reset();
	this->parent = parent;
	statement = tokens;
	if(operators.size() == 0) parent->getRoot()->getOperators(operators);
	resolved = "";
	statement = tokens;
	STRINGS tmp, tmpParam;
	string tmpStr;
	RESULT tmpResult;
	RESULT lastResult;
	std::vector<operation> ops;
	OPERATION_ORDERS opOrders;
	operation tmpOp;
	bool wasLastOp = true;
	//unsigned long x;
	unsigned long idx = 0;

	if(!tokens.size()) return this;

	//ops.push_back(tmpOp);
	while(i < statement.size() && *statement[i] != ',')
	{
		if(!statement.compare(i, "function"))
		{
			symbol* s = parent->getRoot()->createChild();
			s->parse(statement, i);

			unsigned long x;
			for(unsigned long j=0; j < s->arguments.size(); j++)
			{
				x = 0;
				s->arguments[j]->parse(s->arguments[j]->statement, x);
			}

			if(i+1 < statement.size() && !statement.compare(i+1, "{"))
			{
				i++;
				symbol* sc = s->createChild();
				sc->parse(statement, i);

				//s->resolveInheritance(statement);
			}
			fprintf(parent->debug, "lambda call: %s(", s->name.c_str());
			for(long j=0; j < s->arguments.size(); j++)
			{
				if(j == 0) fprintf(parent->debug, "%s", s->arguments[j]->name.c_str());
				else fprintf(parent->debug, ", %s", s->arguments[j]->name.c_str());
			}
			fprintf(parent->debug, ")\n");
			tmpOp.reset();
			tmpOp.sym = s;
			tmpOp.anonymous = true;
			tmpOp.param.push_back(s->name.c_str());
			tmpOp.isOp = 0;
			tmpOp.datatype = parent->findSymbol("void");
			tmpOp.pointer = 1;
			
		}
		else
		{
			fprintf(parent->debug, "op: %s\n", statement[i]);
			if(!statement.compare(i, ";")) break;
			tmp = parseNextSymbol(i);
			tmpOp.reset();
			tmpOp.param = tmp;
			tmpOp.isOp = isOp(*tmp[0]) || !strcmp(tmp[0], "new") || !strcmp(tmp[0], "delete")?1:0;
		}

		if(idx)
		{
			ops[idx-1].next = idx + 1;
			tmpOp.prev = idx;
		}

		ops.push_back(tmpOp);
		idx++;
	}

	for(unsigned long x=0; x < ops.size(); x++)
	{
		if(!ops[x].param.size()) continue;
		unsigned char found = 0;
		fprintf(parent->debug, "%d: %s%s %d - %d\n", x, ops[x].isOp?"%":"", ops[x].param.c_str(), ops[x].prev, ops[x].next);
		if(*ops[x].param[0] == '(')
		{
			RESULT groupResult;
			unsigned long xx = 1;
			fprintf(parent->debug, "group size: %d\n", ops[x].param.size());
			groupResult.parse(parent, ops[x].param, xx);

			for(OPERATOR_DIRECTIONS::iterator it = operators[ops[x].param[0]].begin(); it != operators[ops[x].param[0]].end(); it++)
			{
				if(!it->first || it->first == 1)
				{
					//ops[x].op = it->second;
					opOrders[it->second->precedence].push_back(x);
					//fprintf(parent->debug, "foudn %d %d op, precendence %d\n", it->first % 2, it->first < 2?1:2, it->second->precedence);
					found = 3;
					break;
				}
			}
			wasLastOp = false;
		}
		else if(wasLastOp) // expecting param
		{
			if(ops[x].isOp) // pre-unary-op
			{
				fprintf(parent->debug, "expected a pre-unary operator, got '%s'!\n", ops[x].param.c_str());

				for(OPERATOR_DIRECTIONS::iterator it = operators[ops[x].param[0]].begin(); it != operators[ops[x].param[0]].end(); it++)
				{
					if(!(!it->first || it->first == 1)) continue; // skip non unary ops
					ops[x].op = it->second;
					opOrders[it->second->precedence].push_back(x);
					fprintf(parent->debug, "found unary %d %d op, precendence %d\n", it->first % 2, it->first < 2?1:2, it->second->precedence);
					found = 1;
					ops[x].isOp = 2;
					break;
				}

				if(!found)
				{
					throw "did not find pre-unary op";
				}
				wasLastOp = true;
			}
			else if(ops[x].anonymous)
			{
				ops[x].datatype = ops[x].sym->findSymbol("void"); //functor
				ops[x].param.clear();
				tmpStr = "";
				ops[x].sym->render(tmpStr);
				ops[x].sym->incRefCount(root);
				ops[x].param.push_back(tmpStr);
				ops[x].pointer = 1;
			}
			else // param
			{
				switch(ops[x].param(0))
				{
				case string_number:
					ops[x].isConst = true;
					ops[x].datatype = parent->findSymbol("long");
					ops[x].pointer = 0;
					fprintf(parent->debug, "const ");
					break;
				case string_text:
					ops[x].isConst = true;
					ops[x].datatype = parent->findSymbol("char");
					ops[x].pointer = 1;
					tmpStr = "\"";
					tmpStr += ops[x].param.c_str();
					tmpStr += "\"";
					ops[x].param.clear();
					ops[x].param.push_back(tmpStr);
					fprintf(parent->debug, "const string %s\n", ops[x].param.c_str());
					break;
				default:
					ops[x].sym = parent->findSymbol(ops[x].param[0], false, NULL, &ops[x].scope);
					if(ops[x].sym)
					{
						/*if(ops[x].sym->parent && ops[x].sym->parent->type == STRUCT)
						{
							fprintf(parent->debug, "adding member access: %s\n", ops[x].sym->name.c_str());
							ops[x].base = parent->findSymbol("this");
						}*/
						if(ops[x].sym->isDatatype())
						{
							fprintf(parent->debug, "found datatype: '%s' %d\n", ops[x].param[0], ops[x].sym->type);
							ops[x].datatype = ops[x].sym;
							ops[x].pointer = 0;
							ops[x].param.clear();
							ops[x].param.push_back("NULL");
						}
						else
						{
							ops[x].datatype = ops[x].sym->datatype;
							if(!ops[x].datatype)
							{
								/*string e = "no datatype: ";
								e += ops[x].param[0];
								throw e.c_str();*/
							}
							ops[x].pointer = ops[x].sym->pointer;
						}
						fprintf(parent->debug, "adding symbol[%d] %s %x\n", x, ops[x].sym->name.c_str(), ops[x].datatype);
					}
					else
					{
						//parent->printTree();
						fprintf(parent->debug, "failed to find symbol %s type: %d parent: %x\n", ops[x].param[0], ops[x].param(0), parent);
						//throw "failed to find symbol";
					}
				}
				fprintf(parent->debug, "param: %s\n", ops[x].param[0]);
				wasLastOp = false;
			}
		}
		else //expecting operator
		{
			wasLastOp = true;
			if(ops[x].isOp) // operator
			{
				if(operators.count(ops[x].param[0]))
				{
					for(OPERATOR_DIRECTIONS::iterator it = operators[ops[x].param[0]].begin(); it != operators[ops[x].param[0]].end(); it++)
					{
						if(!it->first) continue; // skip ltr unary ops
						if(it->first == 1) continue; // skip rtl unary ops (for now)
						ops[x].op = it->second;
						opOrders[it->second->precedence].push_back(x);
						fprintf(parent->debug, "found %d %d op, precendence %d\n", it->first % 2, it->first < 2?1:2, it->second->precedence);
						found = 2;
						break;
					}

					if(!found) for(OPERATOR_DIRECTIONS::iterator it = operators[ops[x].param[0]].begin(); it != operators[ops[x].param[0]].end(); it++)
					{
						//if(!(!it->first || it->first == 1)) continue; // skip non unary ops
						ops[x].op = it->second;
						opOrders[it->second->precedence].push_back(x);
						fprintf(parent->debug, "found %d %d op, precendence %d\n", it->first % 2, it->first < 2?1:2, it->second->precedence);
						found = 1;
						break;
					}
					/*if(operators[ops[x].param.c_str()].count(1)) // rtl
					{
					fprintf(parent->debug, "foudn rtl op\n");
					}
					else // ltr;
					{
					fprintf(parent->debug, "foudn ltr op\n");
					}*/
				}
				if(!found)
				{
					fprintf(parent->error, "could not find op: %s\n", ops[x].param[0]);
					for(OPERATORS::iterator it = operators.begin(); it != operators.end(); it++)
					{
						fprintf(parent->error, "checked %s op\n", it->first.c_str());
					}
				}
				//int prec = operators[ops[x].param[0]][idx]->precedence;
				//fprintf(parent->debug, "op precendence: %d\n", prec);
			}
			else // a param?
			{
				if(x > 0)
				{
					fprintf(parent->error, "'%s' is not a declared datatype\n", ops[x-1].param.c_str());
					throw "undeclared datatype";
				}
				else fprintf(parent->error, "expected an operator, got '%s'!\n", ops[x].param.c_str());
			}
		}
		//fprintf(parent->debug, "%d: %x %d - %d\n", i, ops[i].param.c_str(), ops[i].prev, ops[i].next);
	}

	/*for(unsigned long x=0; x < ops.size(); x++)
	{
	fprintf(parent->debug, "-%d: %s%s %d - %d\n", x, ops[x].isOp?"%":"", ops[x].param.c_str(), ops[x].prev, ops[x].next);
	}*/

	for(OPERATION_ORDERS::reverse_iterator it = opOrders.rbegin(); it != opOrders.rend(); it++)
	{
		fprintf(parent->debug, "processing prec: %d\n", it->first);
		for(unsigned long x=0; x < it->second.size(); x++)
		{
			for(unsigned long xx=0; xx < ops.size(); xx++)
			{
				fprintf(parent->debug, "dump[%d] = %s%s %d - %d datatype: %s\n", xx, ops[xx].isOp?"%":"", ops[xx].param.c_str(), ops[xx].prev, ops[xx].next, ops[xx].datatype?ops[xx].datatype->name.c_str():"NULL");
			}

			bool skipSet = false;
			operation* op = &ops[it->second[x]];
			operation* left = NULL;
			if(op->prev > 0) left = &ops[op->prev-1];
			operation* right = NULL;
			if(op->next > 0) right = &ops[op->next-1];
			STRINGS s;
			string leftDatatype, rightDatatype;

			if(!op->param.compare(0, "("))
			{
				bool fptr = false;
				string thisStr = "this";
				STRINGS sargs;
				std::vector<const char*> args;
				unsigned long f_i=0;
				RESULT parenResult[MAX_FUNCTION_PARAMS];
				unsigned long j=0;

				while(++j < op->param.size() && f_i < MAX_FUNCTION_PARAMS)
				{
					if(j != 1)
					{
					}
					parenResult[f_i].parse(parent, op->param, j);
					parent->getDatatype(leftDatatype, parenResult[f_i].datatype?parenResult[f_i].datatype->name.c_str():"NULL", parenResult[f_i].pointer);
					sargs.push_back(leftDatatype.c_str());
					fprintf(parent->debug, "function call! %s '%s'\n", parenResult[f_i].resolved.c_str(), /*parenResult[f_i].datatype->name.c_str()*/ leftDatatype.c_str());
					f_i++;
				}

				for(int j=0; j < sargs.size(); j++)
				{
					args.push_back(sargs[j]);
				}

				symbol* func;

				if(!left->datatype)
				{
					//fprintf(parent->debug, "invalid lparam: %s\n", left->param.c_str());
					//throw "invalid lparam";
				}

				if(left->cache.size() == 0)
				{
					if(!left->sym) // datatype
					{
						if(left->datatype && left->datatype->type == STRUCT) // initializer
						{
							tmpStr = left->datatype->name.c_str();
							tmpStr += "*";
							symbol* newFunc = parent->findFunction("new", NULL, tmpStr.c_str());
							if(newFunc)
							{
								thisStr = "";
								newFunc->render(thisStr, false);
								newFunc->incRefCount(root);
								thisStr += "(NULL)";
							}
							left->cache = "init";
						}
					}
					else if(left->sym->type == VARIABLE || left->sym->isDatatype())
					{
						symbol* ss = NULL;
						fprintf(parent->debug, "found variable function call: '%s'\n", left->sym->name.c_str());

						if(left->sym->isDatatype())
						{
							ss = left->sym;
						}
						else if(left->datatype)
						{
							ss = left->datatype;
						}

						if(ss)
						{
							if(ss->type == STRUCT)
							{
								tmpStr = ss->name.c_str();
								tmpStr += "*";
								symbol* newFunc = parent->findFunction("new", NULL, tmpStr.c_str());
								if(newFunc)
								{
									if(left->base)
									{
										/*thisStr = left->base->name.c_str();
										if(left->base->pointer) thisStr += "->";
										else thisStr += ".";*/
										thisStr = "";
										left->base->renderMember(thisStr);
										thisStr += left->param.c_str();
										left->base = NULL;
									}
									else
									{
										thisStr = left->param.c_str();
									}
									if(thisStr.compare("NULL"))
									{
										thisStr += " = ";
									}
									else
									{
										thisStr = "";
									}
									newFunc->render(thisStr, false);
									newFunc->incRefCount(root);
									thisStr += "(NULL)";
								}
								else
								{
									fprintf(parent->debug, "could not find new %s\n", tmpStr.c_str());
								}
								fprintf(parent->debug, "this = %s\n", thisStr.c_str());
								left->cache = "init";
							}
							else
							{
								fptr = true;
								fprintf(parent->debug, "function pointer!\n");
								//throw "function pointer not supported yet";
							}
						}
						else
						{
							throw "datatype not set";
						}
					}
				}

				if(left->datatype && left->datatype->type == STRUCT && left->cache.size())
				{
					fprintf(parent->debug, "resolving member method for class %s for %s\n", left->datatype->name.c_str(), left->cache.c_str());
					if(thisStr == "this") thisStr = left->param.c_str();
					//symbol* scope = parent->findSymbol(left->datatype?left->datatype->name.c_str():"", false);
					func = left->datatype->findFunction(left->cache.c_str(), NULL, args, false);

					if(!func)
					{
						fprintf(parent->error, "did not find parent struct '%s'.'%s'(", left->datatype?left->datatype->name.c_str():"NULL", left->cache.c_str());
						for(unsigned int i=0; i < args.size(); i++)
						{
							if(i) fprintf(parent->error, ", %s", args[i]);
							else fprintf(parent->error, "%s", args[i]);
						}
						fprintf(parent->error, ")\n");
						throw "failed to find parent struct";
					}
				}
				else if(left->sym && !left->sym->name.compare("typeof"))
                                {
                                        fprintf(parent->debug, "found macro call typeof\n");
                                        tmpStr = "";
                                        if(args.size() == 1)
                                        {
                                                //tmpStr = ultoa((unsigned long)parent, 16);
						//parenResult[0].datatype->render(tmpStr);
                                                //parent->render(tmpStr);
						if(parenResult[0].datatype->type == STRUCT) s.push_back("struct ");
                                                s.push_back(parenResult[0].datatype->name.c_str());
                                                left->datatype = parent->findSymbol("void");
                                                left->pointer = 0;
                                                skipSet = true;
                                        }
                                        else
                                        {
                                                throw "incorrect number of args for typeof";
                                        }
                                }
				else
				{
					if(!left->sym)
					{
						fprintf(parent->error, "NULL datatype and no symbol: %s\n", left->param.c_str());
						throw "failed to find function";
					}
					func = parent->findFunction(left->sym->name.c_str(), NULL, args, true);
				}

				if(!fptr && !func && !skipSet)
				{
					fprintf(parent->error, "did not find func symbol %s %s(%d) %x: ", left->datatype?left->datatype->name.c_str():"NULL", left->sym?left->sym->name.c_str():"NULL", args.size(), func);
					for(unsigned int j=0; j < args.size(); j++)
					{
						fprintf(parent->error, "%s, ", args[j]);
					}
					fprintf(parent->error, "\n");
					throw "could not find function";
				}
				/*if(func->isInline)
				{
					s.push_back("inline");
				}
				else*/
				if(skipSet)
				{
				}
				else
				{
					tmpStr = "";

					if(fptr)
					{
						tmpStr += "((ANON_FUNC_TYPE)";
						tmpStr += left->param.c_str(); 
						tmpStr += ")";
					}
					else
					{
						func->render(tmpStr, false);
						func->incRefCount(root);
					}
					s.push_back(tmpStr);
					s.push_back("(");

					bool shownParent = false;
					if(!fptr && func->parent && func->parent->type == STRUCT)
					{
						if(left->scope.object)
						{
							if(left->scope.object->name.compare("this") || thisStr.compare("this"))
							{
								string tmp = "";
								left->scope.object->renderMember(tmp);
								s.push_back(tmp);
								/*s.push_back(left->scope.object->name.c_str());
								if(left->scope.object->pointer) s.push_back("->");
								else s.push_back(".");*/
							}
							left->scope.reset();
						}
						s.push_back(thisStr);
						shownParent = true;
					}

					for(unsigned long i=0; i < args.size(); i++)
					{
						if(i || shownParent)
						{
							s.push_back(", ");
						}
						s.push_back(parenResult[i].resolved.c_str());
					}
					s.push_back(")");
				}
				left->param = s;
				left->next = op->next;
				if(left->next > 0) ops[left->next-1].prev = op->prev;
				left->isOp = 0;

				if(!skipSet)
				{
					if(fptr)
					{
						left->datatype = parent->findSymbol("void");
						left->pointer = 1;
					}
					else
					{
						left->datatype = func->datatype;
						left->pointer = func->pointer;
					}
				}

				op->reset();
				continue;
			}
			else if(op->isOp == 2) // pre-unary op
			{
				if(!op->param.compare(0, "&"))
				{
					s.push_back("&");
					if(right->scope.object /*&& right->sym->type != FUNCTION*/)
					{
						string tmp = "";
						//s.push_back(right->scope.object->name.c_str());
						left->scope.object->renderMember(tmp);
						s.push_back(tmp);
						//if(right->scope.object->pointer) s.push_back("->");
						//else s.push_back(".");
					}
					/*if(right->base && right->sym->type != FUNCTION)
					{
						s.push_back(right->base->name.c_str());
						s.push_back("->");
					}*/
					s.push_back(right->param);
					op->param = s;
					op->isOp = 0;
					op->next = right->next;

					op->datatype = right->datatype;
					op->pointer = right->pointer+1;
					if(right->next > 0) ops[right->next-1].prev = right->prev;
					right->reset();
					continue;
				}
				else
				{
					tmpStr = op->param.c_str();
					tmpStr += "$";
					parent->getDatatype(rightDatatype, right->datatype->name.c_str(), right->pointer);
					symbol* func = parent->findFunction(tmpStr.c_str(), NULL, rightDatatype.c_str(), true);

					if(!func)
					{
						throw "failed to find pre-unary op";
					}

					if(func->verbatim)
					{
						s.push_back(op->param.c_str());
						s.push_back(right->param);
					}
					else
					{
						tmpStr = "";
						func->render(tmpStr, false);
						func->incRefCount(root);
						s.push_back(tmpStr);
						s.push_back("(");

						if(right->base && right->sym->type != FUNCTION)
						{
							string tmp = "";
							right->base->renderMember(tmp);
							s.push_back(tmp);
							/*s.push_back(right->base->name.c_str());
							if(left->pointer) s.push_back("->");
							else s.push_back(".");*/
						}

						s.push_back(right->param);
						s.push_back(")");
					}

					op->param = s;

					op->isOp = 0;
					op->next = right->next;

					op->datatype = right->datatype;
                                        op->pointer = right->pointer;

					if(right->next > 0) ops[right->next-1].prev = right->prev;
					right->reset();
					continue;
				}
			}
			else
			{
				if(op->next-1 < 0 || op->next-1 >= ops.size())
				{
					fprintf(parent->error, "right is null %d -> %d, prev: %d\n", it->second[x], op->next-1, op->prev-1);
					right = NULL;
					continue;
				}
			}

			left->isOp = 0;
			if(right)
			{
				left->next = right->next;
				if(right->next > 0) ops[right->next-1].prev = op->prev;
			}

			if(!op->param.compare(0, "."))
			{
				if(!left->datatype)
				{
					fprintf(parent->error, "Failed to find class '%s'\n", (left->sym && left->sym->datatype)?left->sym->datatype->name.c_str():"NULL");
					exit(0);
				}
				fprintf(parent->debug, "searching class %s for %s\n", left->datatype->name.c_str(), right?right->param.c_str():"NULL");
				symbol* classSymbol = left->datatype->findLocalSymbol(right->param.c_str());
				if(classSymbol && classSymbol->type != FUNCTION)
				{
					left->param.clear();
					if(!left->sym)
					{
					}
					tmpStr = "";
					left->sym->render(&left->scope, tmpStr);
					left->param.push_back(tmpStr);
					if(left->pointer) left->param.push_back("->");
					else left->param.push_back(".");
					left->param.push_back(right->param);

					left->datatype = classSymbol->datatype;
					left->pointer = classSymbol->pointer;
					left->scope.reset();
				}
				else
				{
					//left->param.clear();
					left->cache = right->param.c_str();
					fprintf(parent->debug, "setting cache = '%s'\n", left->cache.c_str());
					if(left->cache.size() == 0) left->cache = "hmm";
				}
			}
			else
			{
				//left->datatype = right->datatype;
				if(!left) throw "left is empty";
				if(!right) throw "right is empty";

				if(!right->datatype)
				{
					if(left->datatype && op)
					{
						fprintf(parent->error, "could not find right datatype for '%s' '%s' '%s'\n", left->datatype->name.c_str(), op->param.c_str(), right->param.c_str());
						throw "error";
					}
				}
				parent->getDatatype(leftDatatype, left->datatype?left->datatype->name.c_str():"NULL", left->pointer);
				parent->getDatatype(rightDatatype, right->datatype?right->datatype->name.c_str():"NULL", right->pointer);

				fprintf(parent->debug, "locating operator func: '%s'\n", leftDatatype.c_str());
				symbol* func = parent->findFunction(op->param.c_str(), /*leftDatatype.c_str()*/ NULL, leftDatatype.c_str(), rightDatatype.c_str(), true);

				if(!func)
				{
					fprintf(parent->error, "failed to find func %s (%s, %s)\n", op->param.c_str(),  leftDatatype.c_str(), rightDatatype.c_str());
					throw "failed to find function";
				}


				tmpStr = "";
				if(!func->verbatim)
				{
					func->render(tmpStr, false);
					func->incRefCount(root);
					s.push_back(tmpStr);
					s.push_back("(");
				}

				if(func->arguments[0]->reference)
				{
					s.push_back("&");
				}

				if(left->scope.object && left->sym->type != FUNCTION)
				{
					s.push_back(left->scope.object->name.c_str());
					if(left->scope.object->pointer) s.push_back("->");
					else s.push_back(".");
				}

				s.push_back(left->param);

				if(!func->verbatim)
				{
					s.push_back(", ");
				}
				else
				{
					s.push_back(op->param[0]);
				}

				if(right->scope.object && right->sym->type != FUNCTION)
				{
					string tmp = "";
					right->scope.object->renderMember(tmp);
					s.push_back(tmp);
					/*s.push_back(right->scope.object->name.c_str());
					if(right->scope.object->pointer) s.push_back("->");
					else s.push_back(".");*/
				}

				s.push_back(right->param);

				if(!func->verbatim)
				{
					s.push_back(")");
				}
				left->param = s;
				left->base = NULL;
				left->scope.reset();

				//left->datatype = right->datatype;

				if(left->datatype && right->datatype) fprintf(parent->debug, " --------------------- op %s (%s, %s)\n", op->param.c_str(), left->datatype->name.c_str(), right->datatype->name.c_str());
				else
				{
					if(left->datatype) fprintf(parent->debug, " --------------------- op %s (%s, null)\n", op->param.c_str(), left->datatype->name.c_str());
					if(right->datatype) fprintf(parent->debug, " --------------------- op %s (null %s, %s)\n", op->param.c_str(), left->param.c_str(), right->datatype->name.c_str());
				}
			}

			/*for(unsigned long x2=0; x2 < it->second.size(); x2++)
			{
			if(ops[x2].next == it->second[x])
			{
			}
			}*/

			op->reset();
			if(right) right->reset();
			fprintf(parent->debug, "%d\n", it->second[x]);
		}
	}

	for(unsigned long x=0; x < ops.size(); x++)
	{
		fprintf(parent->debug, "%d: %s%s %d - %d\n", x, ops[x].isOp?"%":"", ops[x].param.c_str(), ops[x].prev, ops[x].next);
	}

	if(ops.size())
	{
		datatype = ops[0].datatype;
		pointer = ops[0].pointer;

		if(1 && ops[0].scope.object && ops[0].sym->type != FUNCTION)
		{
			resolved = "";
			ops[0].scope.object->renderMember(resolved);
			/*resolved = ops[0].scope.object->name.c_str();
			if(ops[0].scope.object->pointer) resolved += "->";
			else resolved += ".";*/
			resolved += ops[0].param.c_str();
		}
		/*else if(ops[0].isConst)
		{
			resolved = "const";
		}*/
		else resolved = ops[0].param.c_str();
	}
	else
	{
		resolved = "";
	}
	return this;
}

STRINGS RESULT::parseNextSymbol(unsigned long &i)
{
	STRINGS s;
	if(i >= statement.size()) return s;
	if(!statement.compare(i, "("))
	{
		long open = 1;
		//s.push_back("(");
		while(i < statement.size())
		{
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
			s.push_back((char*)statement[i], statement.length(i), statement(i));
			i++;
		}
	}
	else
	{
		s.push_back((char*)statement[i], statement.length(i), statement(i));
		i++;
	}
	return s;
}
