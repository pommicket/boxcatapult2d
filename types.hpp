#ifndef TYPES_H_
#define TYPES_H_

#if _WIN32
#include <windows.h>
#endif
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdarg.h>

#if __STDC_VERSION__ >= 201112
#include <stdalign.h>
typedef max_align_t MaxAlign;
#else
typedef union {
	long
#if __STDC_VERSION__ >= 199901 || _MSC_VER
	long
#endif
	a;
	long double b;
	void *c;
	void (*d)(void);
} MaxAlign;
#endif

typedef unsigned long ulong;
typedef unsigned uint;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
#define U8_MAX  0xff
#define U16_MAX 0xffff
#define U32_MAX 0xffffffff
#define U64_MAX 0xffffffffffffffff

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
#define I8_MAX  0x7f
#define I16_MAX 0x7fff
#define I32_MAX 0x7fffffff
#define I64_Max 0x7fffffffffffffff

#ifdef __GNUC__
#define no_warn_start _Pragma("GCC diagnostic push") \
    _Pragma("GCC diagnostic ignored \"-Wpedantic\"") \
    _Pragma("GCC diagnostic ignored \"-Wsign-conversion\"") \
    _Pragma("GCC diagnostic ignored \"-Wsign-compare\"") \
    _Pragma("GCC diagnostic ignored \"-Wconversion\"") \
    _Pragma("GCC diagnostic ignored \"-Wimplicit-fallthrough\"") \
    _Pragma("GCC diagnostic ignored \"-Wunused-function\"")

#define no_warn_end _Pragma("GCC diagnostic pop")
#else
#define no_warn_start
#define no_warn_end
#endif

#if DEBUG
#if __unix__
#define logln(...) printf(__VA_ARGS__), printf("\n")
#else // __unix__
static void logln(char const *fmt, ...) {
    char buf[256];
    va_list args;
    va_start(args, fmt);
    vsprintf_s(buf, sizeof buf, fmt, args);
    va_end(args);
    OutputDebugStringA(buf);
    OutputDebugStringA("\n");
}
#endif // __unix__
#else // DEBUG
#define logln(...)
#endif



#endif // TYPES_H_
