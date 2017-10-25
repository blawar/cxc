#include <iostream>
#include "strings.h"

#define STRINGS_BLOCK 1024

#define S(s) boost::lexical_cast<std::string>(s)

STRINGS::STRINGS()
{
	capacity = 1024;
	buf = (unsigned char*)malloc(capacity);
	count = current_i = 0;
	dwPtr.empty();
	if(!buf) throw "Failed to alloc 1024 bytes of memory";
}

STRINGS::~STRINGS()
{
	if(buf)
	{
		free(buf);
		buf = NULL;
		current_i = capacity = 0;
	}
}

const char* STRINGS::operator[](int64 i)
{
	int64 pos = dwPtr[i];
	if(pos+sizeof(STRINGS_NUM)+sizeof(STRING_TYPE) > current_i) throw "string element exception";
	return (const char*)buf+pos+sizeof(STRINGS_NUM)+sizeof(STRING_TYPE);
}

STRING_TYPE STRINGS::operator()(int64 i)
{
	int64 pos = dwPtr[i];
	if(pos+sizeof(STRINGS_NUM)+sizeof(STRING_TYPE) > current_i) throw "string element exception";
	return *(STRING_TYPE*)(buf+pos+sizeof(STRINGS_NUM));
}

void STRINGS::push_back(std::string s, STRING_TYPE type)
{
	push_back(s.c_str(), s.size(), type);
}

void STRINGS::push_back(const char* s, int64 len, STRING_TYPE type)
{
	if(!buf)
	{
		capacity = 1024;
		if(len > capacity) capacity = ((len + 1 + sizeof(STRINGS_NUM)+sizeof(STRING_TYPE)) * 3 / 2) + STRINGS_BLOCK;
		buf = (unsigned char*)malloc(capacity);
		count = current_i = 0;
		dwPtr.empty();
		if(!buf) throw "Failed to alloc 1024 bytes of memory";
	}
	if((current_i + len + 1 + sizeof(STRINGS_NUM)+sizeof(STRING_TYPE)) > capacity)
	{
		capacity = ((current_i + len + 1 + sizeof(STRINGS_NUM)+sizeof(STRING_TYPE)) * 3 / 2) + STRINGS_BLOCK;
		buf = (unsigned char*)realloc(buf, capacity);
		if(!buf)
		{
				throw std::string("Failed realloc strings buffer, tried to allocated: ") + boost::lexical_cast<std::string>(capacity);
		}
	}
	*(STRINGS_NUM*)(buf+current_i) = (STRINGS_NUM)len;
	*(STRING_TYPE*)(buf+current_i+sizeof(STRINGS_NUM)) = type;
	memcpy(buf+current_i+sizeof(STRINGS_NUM)+sizeof(STRING_TYPE), s, (size_t)len);
	*(char*)(buf+current_i+sizeof(STRINGS_NUM)+len+sizeof(STRING_TYPE)) = '\0';
	dwPtr.push_back(current_i);
	count++;
	current_i += sizeof(STRINGS_NUM)+len+1+sizeof(STRING_TYPE);
}

int64 STRINGS::size()
{
	return count;
}

int STRINGS::compare(int64 i, std::string s)
{
	return compare(i, s.c_str());
}

int STRINGS::compare(int64 i, const char* s)
{
	if(!s) throw "tried to compare null string";

	const char* str1=s;
	const char* str2=operator[](i);
	if(!str1 && !str2) return 0;
	if(!str1) return 1;
	if(!str2) return -1;

	while (*str1 && *str2 && tolower((unsigned char)*str1) == tolower((unsigned char)*str2))
	{
		str1++;
		str2++;
	}

	return tolower((unsigned char)*str1) - tolower((unsigned char)*str2);
}

int64 STRINGS::length(int64 i)
{
	return *(STRINGS_NUM*)(buf+ dwPtr[i]);
}

void STRINGS::clear()
{
	count = 0;
	dwPtr.clear();
	current_i = 0;
}

bool STRINGS::reserve(int64 bytes)
{
	if(bytes > capacity)
	{
		capacity = bytes * 5;
		buf = (unsigned char*)realloc(buf, capacity);
		if(!buf)
		{
			throw "Failed realloc strings buffer";
		}
		dwPtr.reserve(bytes / 4);
		return true;
	}
	return false;
}
