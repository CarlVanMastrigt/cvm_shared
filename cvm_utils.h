/**
Copyright 2025 Carl van Mastrigt

This file is part of cvm_shared.

cvm_shared is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

cvm_shared is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with cvm_shared.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <stdint.h>
#include <stdlib.h>
// #include <assert.h>
#include <string.h>
#include <math.h>

static inline char* cvm_strdup(const char * in)
{
    if(!in) return NULL;
    size_t len=(strlen(in)+1)*sizeof(char);
    char * out=malloc(len);
    return memcpy(out,in,len);
}

#ifndef _GNU_SOURCE

// make cvm_ typed?
static inline void sincosf(float angle, float* sin, float* cos)
{
    *sin = sinf(angle);
    *cos = cosf(angle);
}
#endif


#define CVM_INVALID_U32_INDEX 0xFFFFFFFF
#define CVM_INVALID_U16 0xFFFF

#define CVM_CONCAT2_MACRO(A,B) A ## B
#define CVM_CONCAT2(A, B) CVM_CONCAT2_MACRO(A, B)

#define CVM_MAX(A,B) (((A)>(B))?(A):(B))
#define CVM_MIN(A,B) (((A)<(B))?(A):(B))
#define CVM_CLAMP(X,MIN,MAX) CVM_MAX(MIN,CVM_MIN(MAX,X))

static inline uint32_t cvm_align_u32(uint32_t size, uint32_t alignment)
{
    return (size+alignment-1) & ~(alignment-1);
}


/// move these to dedicated file (cvm_math?)
static inline uint64_t cvm_po2_64_gte(uint64_t v){ return 64-__builtin_clzl(v-1); }
static inline uint64_t cvm_po2_64_gt(uint64_t v){ return 64-__builtin_clzl(v); }
static inline uint64_t cvm_po2_64_lt(uint64_t v){ return 63-__builtin_clzl(v-1); }

static inline uint32_t cvm_po2_32_gte(uint32_t v){ return 32-__builtin_clz(v-1); }
static inline uint32_t cvm_po2_32_gt(uint32_t v){ return 32-__builtin_clz(v); }
static inline uint32_t cvm_po2_32_lt(uint32_t v){ return 31-__builtin_clz(v-1); }

static inline uint32_t cvm_lbs_32(uint32_t v){ return __builtin_ctz(v); }
