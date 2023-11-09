#include "utils.h"

//!
//! \brief trim
//! \param in
//! \return
//!
//! Strip trailing spaces and comments from the line
//!
std::string Trim(const std::string& in)
{
    bool inSingleQuote = false;
    bool inDoubleQuote = false;
    bool inEscape = false;

    std::string out;

    for(auto ch : in)
    {
        if (!inSingleQuote && !inDoubleQuote && !inEscape)
        {
            if (ch == ';')
                break;

            switch(ch)
            {
                case '\'':
                    if(!inDoubleQuote)
                    {
                        inSingleQuote = true;
                        out.push_back(ch);
                        continue;
                    }
                    break;
                case '\"':
                    if(!inSingleQuote)
                    {
                        inDoubleQuote = true;
                        out.push_back(ch);
                        continue;
                    }
                    break;
            }
        }

        out.push_back(ch);

        if ((inDoubleQuote || inSingleQuote) && ch == '\\')
        {
            inEscape = true;
            continue;
        }
        if (inSingleQuote && ch == '\'')
        {
            inSingleQuote = false;
            continue;
        }
        if (inDoubleQuote && ch == '\"')
        {
            inDoubleQuote = false;
            continue;
        }
        if (inEscape)
        {
            inEscape = false;
            continue;
        }
    }
    // remove last character if \n or \r (convert MS-DOS line endings)
    while(out.size() > 0 && (out[out.size()-1] == '\r' || out[out.size()-1] == '\n' || out[out.size()-1] == ' ' || out[out.size()-1] == '\t'))
        out.pop_back();
    return out;
}
