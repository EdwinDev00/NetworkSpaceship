//------------------------------------------------------------------------------
//  cvar.cc
//  @copyright (C) 2021 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "config.h"
#include "cvar.h"
#include <unordered_map>
#include <string>
#include <cstring>

namespace Core
{

struct CVarValue
{
    CVarType type;
    union {
        int i;
        float f;
        char* cstr;
    };
};

struct CVar
{
    std::string name;
    std::string description;
    CVarValue value;
    bool modified;
};

constexpr uint16_t MAX_CVARS = 1024;
uint16_t cVarOffset = 0;
CVar cVars[MAX_CVARS];
std::unordered_map<std::string, uint16_t> cVarTable;

//------------------------------------------------------------------------------
/**
*/
CVar*
CVarCreate(CVarCreateInfo const& info)
{
    CVar* ptr = CVarGet(info.name);
    if (ptr) return ptr;

    std::string name = std::string(info.name);
    
#if _DEBUG
    if (name.find(" ") != std::string::npos) {
        printf("CVar name cannot contain spaces.");
        assert(false);
    }
#endif
    
    uint16_t varIndex = cVarOffset++;
    n_assert(varIndex < MAX_CVARS);
    ptr = &cVars[varIndex];
    ptr->name = info.name;
    ptr->value.type = info.type;
    ptr->modified = false;
    if (info.description != nullptr)
        ptr->description = info.description;
    CVarParseWrite(ptr, info.defaultValue);
    cVarTable.insert_or_assign(info.name, varIndex);
    return ptr;
}

//------------------------------------------------------------------------------
/**
*/
CVar*
CVarCreate(CVarType type, const char* name, const char* defaultValue, const char* description)
{
    CVarCreateInfo info;
    info.name = name;
    info.defaultValue = defaultValue;
    info.type = type;
    info.description = description;
    return CVarCreate(info);
}

//------------------------------------------------------------------------------
/**
*/
CVar*
CVarGet(const char* name)
{
    std::string const str = name;
    auto it = cVarTable.find(str);
    if (it != cVarTable.end())
    {
        return &cVars[it->second];
    }

    return nullptr;
}

//------------------------------------------------------------------------------
/**
*/
void
CVarParseWrite(CVar* cVar, const char* value)
{
    switch (cVar->value.type)
    {
    case CVar_Int:
        CVarWriteInt(cVar, atoi(value));
        break;
    case CVar_Float:
        CVarWriteFloat(cVar, (float)atof(value));
        break;
    case CVar_String:
        CVarWriteString(cVar, value);
        break;
    default:
        break;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
CVarWriteFloat(CVar* cVar, float value)
{
    if (cVar->value.type == CVar_Float)
    {
        cVar->value.f = value;
        cVar->modified = true;
        return;
    }

    n_printf("Warning: Invalid CVar type passed to CVarWrite.\n");
    return;
}

//------------------------------------------------------------------------------
/**
*/
void
CVarWriteInt(CVar* cVar, int value)
{
    if (cVar->value.type == CVar_Int)
    {
        cVar->value.i = value;
        cVar->modified = true;
        return;
    }

    n_printf("Warning: Invalid CVar type passed to CVarWrite.\n");
    return;
}

//------------------------------------------------------------------------------
/**
*/
void
CVarWriteString(CVar* cVar, const char* value)
{
    if (cVar->value.type == CVar_String)
    {
        if (cVar->value.cstr)
            free(cVar->value.cstr);
#ifdef _MSC_VER
        cVar->value.cstr = _strdup(value);
#else            
        cVar->value.cstr = strdup(value);
#endif        
        cVar->modified = true;
        return;
    }

    n_printf("Warning: Invalid CVar type passed to CVarWrite.\n");
    return;
}

//------------------------------------------------------------------------------
/**
*/
int const
CVarReadInt(CVar* cVar)
{
    if (cVar->value.type == CVar_Int)
    {
        return cVar->value.i;
    }

    n_printf("Warning: Trying to read incorrect type from CVar.\n");
    return 0;
}

//------------------------------------------------------------------------------
/**
*/
float const
CVarReadFloat(CVar* cVar)
{
    if (cVar->value.type == CVar_Float)
    {
        return cVar->value.f;
    }

    n_printf("Warning: Trying to read incorrect type from CVar.\n");
    return 0.0f;
}

//------------------------------------------------------------------------------
/**
*/
const char*
CVarReadString(CVar* cVar)
{
    if (cVar->value.type == CVar_String)
    {
        return cVar->value.cstr;
    }

    n_printf("Warning: Trying to read incorrect type from CVar.\n");
    return nullptr;
}

//------------------------------------------------------------------------------
/**
*/
bool
CVarModified(CVar* cVar)
{
    return cVar->modified;
}

//------------------------------------------------------------------------------
/**
*/
void
CVarSetModified(CVar* cVar, bool value)
{
    cVar->modified = value;
}

//------------------------------------------------------------------------------
/**
*/
CVarType
CVarGetType(CVar* cVar)
{
    return cVar->value.type;
}

//------------------------------------------------------------------------------
/**
*/
const char*
CVarGetName(CVar* cVar)
{
    return cVar->name.c_str();
}

//------------------------------------------------------------------------------
/**
*/
const char*
CVarGetDescription(CVar* cVar)
{
    return cVar->description.c_str();
}

//------------------------------------------------------------------------------
/**
*/
int
CVarNum()
{
    return cVarOffset;
}

//------------------------------------------------------------------------------
/**
*/
CVar*
CVarsBegin()
{
    return cVars;
}

//------------------------------------------------------------------------------
/**
*/
CVar*
CVarsEnd()
{
    return cVars + cVarOffset;
}

//------------------------------------------------------------------------------
/**
*/
CVar*
CVarNext(CVar* cVar)
{
    return cVar + 1;
}


} // namespace Core
