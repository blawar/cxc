#ifndef DATATYPES_H
#define DATATYPES_H

#include <map>
#include <string>

#define int64   uint64_t
#define int32   uint32_t
#define int16   uint16_t
#define BYTE unsigned char

enum
{
	APP_NONE,
	APP_MUSIC,
	APP_PLAYER,
	APP_CHAT,
	APP_SOCIAL
};

template<class T>
class ARRAY
{
public:
        T& operator[](std::string key)
        {
                if(!map.count(key)) io.push_back(key);
                return map[key];

        }

        long size()
        {
                return io.size();
        }

        bool empty()
        {
                return io.empty();
        }

        T& operator[](long i)
        {
                return map[io[i]];
        }

        std::string& key(long i)
        {
                return io[i];
        }
private:
        std::vector<std::string> io;
        std::map<std::string, T> map;
};

#endif
