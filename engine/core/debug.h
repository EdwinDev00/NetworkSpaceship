#pragma once
//------------------------------------------------------------------------------
/**
    @file debug.h

    Debug macros.
  
    n_assert()  - the vanilla assert() Macro
    n_assert2() - an assert() plus a message from the programmer
    
    @copyright
    (C) 2002 RadonLabs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/

#ifdef __GNUC__
#define N_PRINT_ATTRIBUTES __attribute__((format(printf,1,2)))
#else
#define N_PRINT_ATTRIBUTES
#endif

void n_printf(const char *, ...) N_PRINT_ATTRIBUTES;
void n_error(const char*, ...) N_PRINT_ATTRIBUTES;
void n_warning(const char*, ...) N_PRINT_ATTRIBUTES;
void n_barf(const char *, const char *, int);
void n_barf2(const char*, const char*, const char*, int);

#if __NO_ASSERT__
#define n_assert(exp) if(!(exp)){}
#define n_assert2(exp, msg) if(!(exp)){}
#define n_assert_fmt(exp, msg, ...) if(!(exp)){}
#else
#define n_assert(exp) { if (!(exp)) n_barf(#exp, __FILE__, __LINE__); }
#define n_assert2(exp, msg) { if (!(exp)) n_barf2(#exp, msg, __FILE__, __LINE__); }
#endif

//------------------------------------------------------------------------------
