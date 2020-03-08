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




#define CVM_UINT_MAX_FLOAT (1.0/((double)UINT_MAX))

static inline unsigned int rng_lcg0(unsigned int b)
{
    return b*1103515245+12345;
}
static inline unsigned int rng_lcg0_uint(unsigned int * seed)
{
    return (*seed=rng_lcg0(*seed));
}
static inline float rng_lcg0_float(unsigned int * seed)
{
    return (float)(((double)rng_lcg0_uint(seed))*CVM_UINT_MAX_FLOAT);
}



static inline unsigned int rng_lcg1(unsigned int b)
{
    return b*1664525+1013904223;
}
static inline unsigned int rng_lcg1_uint(unsigned int * seed)
{
    return (*seed=rng_lcg1(*seed));
}
static inline float rng_lcg1_float(unsigned int * seed)
{
    return (float)(((double)rng_lcg1_uint(seed))*CVM_UINT_MAX_FLOAT);
}



static inline unsigned int rng_lcg2(unsigned int b)
{
    return b*134775813+1;
}
static inline unsigned int rng_lcg2_uint(unsigned int * seed)
{
    return (*seed=rng_lcg2(*seed));
}
static inline float rng_lcg2_float(unsigned int * seed)
{
    return (float)(((double)rng_lcg2_uint(seed))*CVM_UINT_MAX_FLOAT);
}



static inline unsigned int rng_lfsr0(unsigned int b)
{
    return ((b>>1)|( ((b) ^ (b>>2) ^ (b>>6) ^ (b>>7)) <<31));
}
static inline unsigned int rng_lfsr0_uint(unsigned int * seed)
{
    return (*seed=rng_lfsr0(*seed));
}
static inline float rng_lfsr0_float(unsigned int * seed)
{
  return (float)(((double)rng_lfsr0_uint(seed))*CVM_UINT_MAX_FLOAT);
}



static inline unsigned int rng_lfsr1(unsigned int b)
{
    return ((b>>1)|( ((b) ^ (b>>7) ^ (b>>10) ^ (b>>17)) <<31));
}
static inline unsigned int rng_lfsr1_uint(unsigned int * seed)
{
    return (*seed=rng_lfsr1(*seed));
}
static inline float rng_lfsr1_float(unsigned int * seed)
{
  return (float)(((double)rng_lfsr1_uint(seed))*CVM_UINT_MAX_FLOAT);
}




static inline unsigned int rng_lfsr2(unsigned int b)
{
    return ((b>>1)|( ((b) ^ (b>>13) ^ (b>>14) ^ (b>>19)) <<31));
}
static inline unsigned int rng_lfsr2_uint(unsigned int * seed)
{
    return (*seed=rng_lfsr2(*seed));
}
static inline float rng_lfsr2_float(unsigned int * seed)
{
  return (float)(((double)rng_lfsr2_uint(seed))*CVM_UINT_MAX_FLOAT);
}




#endif




