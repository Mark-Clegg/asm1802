#include "blob.h"

blob::blob() : Master(true), Relocatable(false), Code({{0, {}}})
{
    CurrentCode = Code.begin();
}

blob::blob(bool Relocatable) : Master(false), Relocatable(Relocatable), Code({{0, {}}})
{
    CurrentCode = Code.begin();
}

blob::blob(bool Relocatable, uint16_t Address) : Master(false), Relocatable(Relocatable), Code({{ Address, {}}})
{
    CurrentCode = Code.begin();
}
