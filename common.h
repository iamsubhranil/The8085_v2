#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdbool.h>


typedef int64_t i64;
#define Pi64 PRId64
#define Si64 SCNd64
#define i64_MAX INT64_MAX
#define i64_MIN INT64_MIN

typedef uint64_t u64;
#define Pu64 PRIu64
#define Su64 SCNu64
#define u64_MAX UINT64_MAX
#define u64_MIN UINT64_MIN

typedef int32_t i32;
#define Pi32 PRId32
#define Si32 SCNd32
#define i32_MAX INT32_MAX
#define i32_MIN INT32_MIN

typedef uint32_t u32;
#define Pu32 PRIu32
#define Su32 SCNu32
#define u32_MAX UINT32_MAX
#define u32_MIN UINT32_MIN

typedef int16_t i16;
#define Pi16 PRId16
#define Si16 SCNd16
#define i16_MAX INT16_MAX
#define i16_MIN INT16_MIN

typedef uint16_t u16;
#define Pu16 PRIu16
#define Su16 SCNu16
#define u16_MAX UINT16_MAX
#define u16_MIN UINT16_MIN

typedef int8_t i8;
#define Pi8 PRId8
#define Si8 SCNd8
#define i8_MAX INT8_MAX
#define i8_MIN INT8_MIN

typedef uint8_t u8;
#define Pu8 PRIu8
#define Su8 SCNu8
#define u8_MAX UINT8_MAX
#define u8_MIN UINT8_MIN

typedef size_t siz;
#define Psiz "zd"
#define Ssiz "zd"

#define STACKTRACE_SHOW

#ifdef __ANDROID__ // no execinfo.h
#undef STACKTRACE_SHOW
#endif

#ifndef USE_BCVM
#define USE_NEOVM
#endif
