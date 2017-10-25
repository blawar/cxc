#ifndef STRINGS_H
#define STRINGS_H

#include <vector>
#include <string>
#include <string.h>
#include <boost/lexical_cast.hpp>
#include "datatypes.h"

enum STRING_TYPE
{
	string_text,
	string_field,
	string_field_compound,
	string_field_array,
	string_number
};

#define STRINGS_NUM	int32_t

class STRINGS
{
public:
	STRINGS();
	~STRINGS();
	const char* operator[](int64 i);
	STRING_TYPE operator()(int64 i);
	void push_back(std::string s, STRING_TYPE type=string_text);
	void push_back(const char* s, int64 len, STRING_TYPE type=string_text);
	int64 size();
	int compare(int64 i, std::string s);
	int compare(int64 i, const char* s);
	int64 length(int64 i);
	void clear();
	bool reserve(int64 bytes);
private:
	int64 capacity;
	unsigned char* buf;
	int64 current_i, count;
	std::vector<int64> dwPtr;
};

#endif
