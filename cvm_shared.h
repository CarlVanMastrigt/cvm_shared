/**
Copyright 2020,2021,2022,2023 Carl van Mastrigt

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

#ifndef CVM_SHARED_H
#define CVM_SHARED_H

///needed for sincosf, may want to re-evaluate usage, at least move to math??
#define _GNU_SOURCE

///change to only include external headers that are actually needed try to reduce count overall
#include <stdbool.h>
#include <stdio.h>
#include <stdatomic.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <threads.h>
#include <time.h>
#include <assert.h>
#include <stdalign.h>

#include <immintrin.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_ADVANCES_H

#include <vulkan/vulkan.h>

///own headers

#define CVM_SHARED_ENGINE_NAME "cvm_shared"
#define CVM_SHARED_ENGINE_VERSION 1

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

typedef union widget widget;

/// move these to dedicated file (cvm_math?)
static inline uint64_t cvm_po2_64_gte(uint64_t v){ return 64-__builtin_clzl(v-1); }
static inline uint64_t cvm_po2_64_gt(uint64_t v){ return 64-__builtin_clzl(v); }
static inline uint64_t cvm_po2_64_lt(uint64_t v){ return 63-__builtin_clzl(v-1); }

static inline uint32_t cvm_po2_32_gte(uint32_t v){ return 32-__builtin_clz(v-1); }
static inline uint32_t cvm_po2_32_gt(uint32_t v){ return 32-__builtin_clz(v); }
static inline uint32_t cvm_po2_32_lt(uint32_t v){ return 31-__builtin_clz(v-1); }



static inline uint32_t cvm_lbs_32(uint32_t v){ return __builtin_ctz(v); }

#include "cvm_utils.h"
#include "cvm_math.h"
#include "cvm_data_structures.h"
#include "cvm_random.h"
#include "cvm_vk.h"
#include "cvm_overlay.h"
#include "cvm_widget.h"
#include "cvm_transform.h"
#include "cvm_camera.h"
#include "cvm_mesh.h"
#include "cvm_coherent_data_structures.h"
#include "cvm_thread.h"
#include "cvm_sync_primitives.h"


#include "cvm_debug_render.h"

#include "cvm_independent_overlay.h"


#endif

