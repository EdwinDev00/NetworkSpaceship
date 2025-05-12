#pragma once

//------------------------------------------------------------------------------
/**
    @file core/config.h
    
	Main configure file for types and OS-specific stuff.
	
	(C) 2015-2022 See the LICENSE file.
*/
#ifdef __WIN32__
#include "win32/pch.h"
#endif
#define NOMINMAX

#include <stdint.h>
#include <atomic>
#include <xmmintrin.h>
#include <assert.h>
#include <vec3.hpp> // glm::vec3
#include <vec4.hpp> // glm::vec4
#include <mat4x4.hpp> // glm::mat4
#include "gtc/matrix_transform.hpp" // glm::translate, glm::rotate, glm::scale, glm::perspective
#include "gtx/transform.hpp"
#include "gtc/quaternion.hpp"
#include "core/debug.h"

typedef size_t index_t;
typedef unsigned int uint;
typedef unsigned short ushort;

typedef uint64_t	uint64;
typedef int64_t	    int64;
typedef uint32_t	uint32;
typedef int32_t		int32;
typedef uint16_t	uint16;
typedef int16_t		int16;
typedef uint8_t		uint8;
typedef int8_t		int8;
typedef uint8_t		uchar;

// eh, windows already defines byte, so don't redefine byte if we are running windows
#ifndef __WIN32__
typedef uint8_t      byte;
#endif

typedef uint8_t		ubyte;
typedef float		float32;
typedef double		float64;

#define j_min(x, y) x < y ? x : y
#define j_max(x, y) x > y ? x : y

#ifdef NULL
#undef NULL
#define NULL nullptr
#endif

// GCC settings
#if defined __GNUC__
#define __cdecl
#define __forceinline inline __attribute__((always_inline))
#pragma GCC diagnostic ignored "-Wmultichar"
#pragma GCC diagnostic ignored "-Wformat-security"
#endif

#if !defined(__GNUC__)
#define  __attribute__(x)  /**/
#endif
