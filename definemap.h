#ifndef DEFINEMAP_H
#define DEFINEMAP_H

#include <algorithm>
#include <map>
#include <string>
#include <vector>

//!
//! \brief The DefineMap class
//!
//! Variant of std::map where iteration yields entries in the same order they were added
//!
class DefineMap
{
private:
    using map_t = std::map<std::string,std::string>;
    using keys_t = std::vector<std::string>;
    //using iterator = keys_t::iterator;
    using const_iterator = keys_t::const_iterator;

    map_t kvp;
    keys_t keys;

public:

    DefineMap();

    //iterator begin() { return keys.begin(); }
    //iterator end() { return keys.end(); }
    const_iterator begin() const
    {
        return keys.begin();
    }
    const_iterator end() const
    {
        return keys.end();
    }
    const_iterator cbegin() const
    {
        return keys.cbegin();
    }
    const_iterator cend() const
    {
        return keys.cend();
    }

    std::string& operator[](const std::string& key);
    bool contains(const std::string& key);
    void erase(const std::string& key);
};

#endif // DEFINEMAP_H
