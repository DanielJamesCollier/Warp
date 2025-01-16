#ifndef DJC_H_
#define DJC_H_

#include <stdint.h>

#define internal static
#define persist static
#define merge inline
#define fold __forceinline
#define noalias restrict

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;
typedef float f32;
typedef double f64;
typedef int8_t b8;
typedef int16_t b16;
typedef int32_t b32;
typedef int64_t b64;

_Static_assert(sizeof(u8) == 1);
_Static_assert(sizeof(u16) == 2);
_Static_assert(sizeof(u32) == 4);
_Static_assert(sizeof(u64) == 8);
_Static_assert(sizeof(s8) == 1);
_Static_assert(sizeof(s16) == 2);
_Static_assert(sizeof(s32) == 4);
_Static_assert(sizeof(s64) == 8);
_Static_assert(sizeof(f32) == 4);
_Static_assert(sizeof(f64) == 8);
_Static_assert(sizeof(b8) == 1);
_Static_assert(sizeof(b16) == 2);
_Static_assert(sizeof(b32) == 4);
_Static_assert(sizeof(b64) == 8);
_Static_assert(sizeof(size_t) == 8);

#endif  // DJC_H_
