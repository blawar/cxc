#include "tokenize.h"

bool tokenize(const char* pszStr, STRINGS &tokens)
{
        std::string token;

        char delimiters[3][2] =
        {
        { '"', '"' },
        { '\'', '\'' },
        { '`', '`' } };
        int delimiter_active = -1;

        for (long i = 0; pszStr[i]; i++)
        {
                if (delimiter_active != -1)
                {
                        if(pszStr[i] == '\\')
                        {
                                token += pszStr[++i];
                                continue;
                        }
                        else if (pszStr[i] == delimiters[delimiter_active][1])
                        {
                                if(pszStr[i+1] != delimiters[delimiter_active][1])
                                {
                                        switch(delimiter_active)
                                        {
                                        case '`':
                                                tokens.push_back(token, string_field);
                                                break;
                                        case '"':
                                                tokens.push_back(token, string_text);
                                                break;
                                        default:
                                                tokens.push_back(token);
                                        }
                                        token = "";
                                        delimiter_active = -1;
                                        continue;
                                }
                                i++;

                        }
                        token += pszStr[i];
                        continue;
                }

                for (int d = 0; d < 3; d++)
                {
                        if (pszStr[i] == delimiters[d][0])
                        {
                                delimiter_active = d;
                                break;
                        }
                }

                if (delimiter_active != -1)
                        continue;

                if(pszStr[i] == '-' && pszStr[i+1] == '-' )
                {
                        while(pszStr[i] && pszStr[i] != '\n') i++;
                        continue;
                }

                if(pszStr[i] == '/' && pszStr[i+1] == '*' )
                {
                        while(pszStr[i] && !(pszStr[i] == '*' && pszStr[i+1] == '/')) i++;
                        if(pszStr[i]) i++;
                        continue;
                }
                switch (pszStr[i])
                {
                case ' ':
                case '\t':
                case '\n':
                case '\r':
                        if (token.length())
                        {
                                if (token.length())
                                {
                                        tokens.push_back(token);
                                }
                                token = "";
                        }
                        break;
                case ';':
                case '=':
                case '+':
                case '-':
                case '/':
                case '*':
                case '!':
                case ',':
                case '(':
                case ')':
		case ':':
                //case '.':
                        if (token.length())
                        {
                                tokens.push_back(token);
                        }
                        token = pszStr[i];
                        tokens.push_back(token);
                        token = "";
                        break;
                /*case '#':
                        {
                                int64 nLength = boost::lexical_cast<int64>(token);

                                token.resize(nLength);
                                i++;
                                if(pszStr[i + nLength] != '#')
                                {
                                        throw std::string("Improper binary encoding");
                                }
                                memcpy((void*)token.c_str(), (void*)(pszStr+i), nLength);
                                tokens.push_back(token);
                                token = "";

                                i += nLength;
                        }
                        break;*/
                default:
                        token += tolower(pszStr[i]);
                        break;
                }
        }
        if(token.size()) tokens.push_back(token);

        //error(string("Tokens: ") + boost::lexical_cast<string>(tokens.size()));

        /*for(long i=0; i < tokens.size(); i++)
        {
                error(tokens[i]);
        }*/
        return true;
}
