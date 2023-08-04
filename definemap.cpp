#include "definemap.h"

DefineMap::DefineMap()
{

}

std::string& DefineMap::operator[](const std::string& key)
{
    if(std::find(keys.begin(), keys.end(), key) == keys.end())
        keys.push_back(key);

    return kvp[key];
}

bool DefineMap::contains(const std::string& key)
{
    std::map<std::string, std::string>::iterator i = kvp.find(key);
    return i != kvp.end();
}

void DefineMap::erase(const std::string& key)
{
    auto pos = std::find(keys.begin(), keys.end(), key);
    if(pos != keys.end())
        keys.erase(pos);
    kvp.erase(key);
}
