//------------------------------------------------------------------------------
// debug.cc
//  (C) 2002 RadonLabs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "config.h"
#include <string>
#include <cstdarg>
//------------------------------------------------------------------------------
/**
    This function is called by n_assert() when the assertion fails.
*/
void 
n_barf(const char* exp, const char* file, int line)
{
    std::string msg = "*** ASSERTION ***\n";
    msg += file;
    msg += "("; msg += line; msg += ")\n";
    msg += "expression: ";
    msg += exp;
    msg += "\n";
    n_error("%s", msg.c_str());
}

//------------------------------------------------------------------------------
/**
    This function is called by n_assert2() when the assertion fails.
*/
void
n_barf2(const char* exp, const char* msg, const char* file, int line)
{
    std::string str = "*** ASSERTION ***\n";
    str += file;
    str += "("; str += line; str += ")\n";
    str += "expression: ";
    str += exp;
    str += "\n";
    str += "programmer says: ";
    str += msg;
    str += "\n";
    n_error("%s", str.c_str());
}

//------------------------------------------------------------------------------
/**
    This function is called when a serious situation is encountered which
    requires abortion of the application.
*/
void __cdecl
n_error(const char* msg, ...)
{
    va_list argList;
    va_start(argList, msg);
    vprintf(msg, argList);
    va_end(argList);
    assert(0);
}

//------------------------------------------------------------------------------
/**
    This function is called when a warning should be issued which doesn't
    require abortion of the application.
*/
void __cdecl
n_warning(const char* msg, ...)
{
    va_list argList;
    va_start(argList, msg);
    printf("[WARNING] ");
    vprintf(msg, argList);
    va_end(argList);
}        

//------------------------------------------------------------------------------
/**
*/
void __cdecl
n_printf(const char *msg, ...)
{
    va_list argList;
    va_start(argList, msg);
    vprintf(msg, argList);
    va_end(argList);
}
