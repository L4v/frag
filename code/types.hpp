#ifndef TYPES_HPP
#define TYPES_HPP
#include <cstdint>

#define global static
#define internal static

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float r32;
typedef double r64;
typedef u32 b32;

const char PATH_SEPARATOR =
#ifdef _WIN32
'\\';
#else
'/';
#endif

#endif
