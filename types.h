#ifndef __TYPES_H__
#define __TYPES_H__

#include <stdint.h>
#include <stdatomic.h>

#define u64 uint64_t
#define s64 int64_t
#define u32 uint32_t
#define s32 int32_t
#define u16 uint16_t
#define s16 int16_t
#define u8  uint8_t
#define s8  int8_t

#define au64 _Atomic uint64_t
#define as64 _Atomic int64_t
#define au32 _Atomic uint32_t
#define as32 _Atomic int32_t
#define au16 _Atomic uint16_t
#define as16 _Atomic int16_t
#define au8  _Atomic uint8_t
#define as8  _Atomic int8_t

#endif
