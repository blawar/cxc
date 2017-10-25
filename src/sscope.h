#ifndef SSCOPE_H
#define SSCOPE_H

class symbol;

enum
{
        SCOPE_NONE,
        SCOPE_LOCAL,
        SCOPE_CLASS,
        SCOPE_GLOBAL
};

struct sscope
{
        char type;
        symbol* root;
        symbol* object;

        sscope()
        {
		reset();
        }

	void reset()
	{
                type = 0;
                root = NULL;
                object = NULL;
	}
};

#endif
