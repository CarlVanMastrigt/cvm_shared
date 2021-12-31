/**
Copyright 2020 Carl van Mastrigt

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
#include "cvm_shared.h"
#endif


#ifndef CVM_RANDOM_H
#define CVM_RANDOM_H




#define CVM_INV_UINT_MAX_FLOAT (1.0/4294967295.0)

///following needs testing
static inline vec3f cvm_rng_point_on_sphere(float rn1,float rn2)
{
    float z;
    z=acosf(rn1*2.0-1.0)*PI_RCP*2.0-1.0;
    rn1=sqrtf(1.0-z*z);
    rn2*=TAU;
    return (vec3f){ .x=rn1*cosf(rn2),.y=rn1*sinf(rn2),.z=z};
}

static inline vec3f cvm_rng_point_in_ball(float rn1,float rn2,float rn3)
{
    float z;
    rn1=cbrtf(rn1);///radius
    z=rn1*acosf(rn2*2.0-1.0)*PI_RCP*2.0-1.0;
    rn2=rn1*sqrtf(1.0-z*z);
    rn3*=TAU;
    return (vec3f){ .x=rn2*cosf(rn3),.y=rn2*sinf(rn3),.z=z};
}

static inline vec2f cvm_rng_point_on_circle(float rn1)
{
    rn1*=TAU;
    return (vec2f){ .x=cosf(rn1),.y=sinf(rn1)};
}

static inline vec2f cvm_rng_point_in_disc(float rn1,float rn2)
{
    rn1=sqrtf(rn1);///radius
    rn2*=TAU;
    return (vec2f){ .x=rn1*cosf(rn2),.y=rn1*sinf(rn2)};
}





static inline uint32_t cvm_rng_lcg0(uint32_t b)
{
    return b*1103515245+12345;
}
static inline uint32_t cvm_rng_lcg0_uint(uint32_t * seed)
{
    return (*seed=cvm_rng_lcg0(*seed));
}
static inline float cvm_rng_lcg0_float(uint32_t * seed)
{
    return (float)(((double)cvm_rng_lcg0_uint(seed))*CVM_INV_UINT_MAX_FLOAT);
}


static inline uint32_t cvm_rng_lcg1(uint32_t b)
{
    return b*1664525+1013904223;
}
static inline uint32_t cvm_rng_lcg1_uint(uint32_t * seed)
{
    return (*seed=cvm_rng_lcg1(*seed));
}
static inline float cvm_rng_lcg1_float(uint32_t * seed)
{
    return (float)(((double)cvm_rng_lcg1_uint(seed))*CVM_INV_UINT_MAX_FLOAT);
}



static inline uint32_t cvm_rng_lcg2(uint32_t b)
{
    return b*134775813+1;
}
static inline uint32_t cvm_rng_lcg2_uint(uint32_t * seed)
{
    return (*seed=cvm_rng_lcg2(*seed));
}
static inline float cvm_rng_lcg2_float(uint32_t * seed)
{
    return (float)(((double)cvm_rng_lcg2_uint(seed))*CVM_INV_UINT_MAX_FLOAT);
}



static inline uint32_t cvm_rng_lfsr0(uint32_t b)
{
    return ((b>>1)|( ((b) ^ (b>>2) ^ (b>>6) ^ (b>>7)) <<31));
}
static inline uint32_t cvm_rng_lfsr0_uint(uint32_t * seed)
{
    return (*seed=cvm_rng_lfsr0(*seed));
}
static inline float cvm_rng_lfsr0_float(uint32_t * seed)
{
  return (float)(((double)cvm_rng_lfsr0_uint(seed))*CVM_INV_UINT_MAX_FLOAT);
}



static inline uint32_t cvm_rng_lfsr1(uint32_t b)
{
    return ((b>>1)|( ((b) ^ (b>>7) ^ (b>>10) ^ (b>>17)) <<31));
}
static inline uint32_t cvm_rng_lfsr1_uint(uint32_t * seed)
{
    return (*seed=cvm_rng_lfsr1(*seed));
}
static inline float rng_lfsr1_float(uint32_t * seed)
{
  return (float)(((double)cvm_rng_lfsr1_uint(seed))*CVM_INV_UINT_MAX_FLOAT);
}




static inline uint32_t cvm_rng_lfsr2(uint32_t b)
{
    return ((b>>1)|( ((b) ^ (b>>13) ^ (b>>14) ^ (b>>19)) <<31));
}
static inline uint32_t cvm_rng_lfsr2_uint(uint32_t * seed)
{
    return (*seed=cvm_rng_lfsr2(*seed));
}
static inline float cvm_rng_lfsr2_float(uint32_t * seed)
{
  return (float)(((double)cvm_rng_lfsr2_uint(seed))*CVM_INV_UINT_MAX_FLOAT);
}






#endif




