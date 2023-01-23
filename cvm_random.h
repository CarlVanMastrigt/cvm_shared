/**
Copyright 2020-2022 Carl van Mastrigt

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


/// following is adapted from https://www.pcg-random.org
/// relevant code can be found at  https://github.com/imneme/pcg-c-basic  and  https://github.com/imneme/pcg-c
/// functions renamed to match scheme used here

/// multiple variants (distinguished by varying increment factor) included to allow diverging seeding


/// useful to have this separately for use with advance functions, or if seed is stored but not result (though that would a strange use case)
static inline uint32_t cvm_rng_pcg_get(uint64_t s)
{
    uint32_t xorshifted = ((s >> 18u) ^ s) >> 27u;
    uint32_t rot = s >> 59u;
    return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}

static inline uint32_t cvm_rng_pcg_a(uint64_t * state)
{
    uint64_t s = *state;
    // Advance internal state
    *state = *state * 0x5851F42D4C957F2D + 0x7A4111AC0FFEE60D;
    // Calculate output function (XSH RR), uses old state for max ILP
    return cvm_rng_pcg_get(s);
}

static inline uint32_t cvm_rng_pcg_b(uint64_t * state)
{
    uint64_t s = *state;
    // Advance internal state
    *state = *state * 0x5851F42D4C957F2D + 0xA1178A7600D5817;
    // Calculate output function (XSH RR), uses old state for max ILP
    return cvm_rng_pcg_get(s);
}

static inline uint32_t cvm_rng_pcg_c(uint64_t * state)
{
    uint64_t s = *state;
    // Advance internal state
    *state = *state * 0x5851F42D4C957F2D + 0x1337A5511CCE25;
    // Calculate output function (XSH RR), uses old state for max ILP
    return cvm_rng_pcg_get(s);
}

static inline uint64_t cvm_rng_pcg_advance(uint64_t state, uint64_t delta,uint64_t cur_plus)
{
    uint64_t cur_mult=0x5851F42D4C957F2D;
    uint64_t acc_mult = 1u;
    uint64_t acc_plus = 0u;
    while (delta)
    {
        if (delta & 1)
        {
            acc_mult *= cur_mult;
            acc_plus = acc_plus * cur_mult + cur_plus;
        }
        cur_plus = (cur_mult + 1) * cur_plus;
        cur_mult *= cur_mult;
        delta >>= 1;
    }
    return acc_mult * state + acc_plus;
}

/// not sure these warrant use, but they're here
static inline uint64_t cvm_rng_pcg_a_advance(uint64_t state, uint64_t delta)
{
    return cvm_rng_pcg_advance(state,delta,0x7A4111AC0FFEE60D);
}

static inline uint64_t cvm_rng_pcg_b_advance(uint64_t state, uint64_t delta)
{
    return cvm_rng_pcg_advance(state,delta,0xA1178A7600D5817);
}

static inline uint64_t cvm_rng_pcg_c_advance(uint64_t state, uint64_t delta)
{
    return cvm_rng_pcg_advance(state,delta,0x1337A5511CCE25);
}



static inline uint32_t cvm_rng_lcg_32(uint32_t * state)
{
    return (*state = *state * 0x2C9277B5 + 0xAC564B05);
}

///for use in 65536 valued 0-1 float rng's as well as any use case where a small seed value is adequate
static inline uint16_t cvm_rng_pcg_16_get(uint32_t s)
{
    return (uint16_t)(((s >> 11u) ^ s) >> ((s >> 30u) + 11u));
}

static inline uint16_t cvm_rng_pcg_16(uint32_t * state)
{
    uint32_t s = *state;
    cvm_rng_lcg_32(state);
    return cvm_rng_pcg_16_get(s);
}


/// end of stuff adapted from https://www.pcg-random.org



static inline float cvm_rng_float_16(uint32_t * s)
{
    return (((float)cvm_rng_pcg_16(s))+0.5f)*0.0000152587890625f;/// 1/65536
}

///can be called instead of above for last use of a particular sub seed (as seen below)
static inline float cvm_rng_float_16_get(uint32_t s)
{
    return (((float)cvm_rng_pcg_16_get(s))+0.5f)*0.0000152587890625f;/// 1/65536
}

///following needs testing
static inline vec3f cvm_rng_point_on_sphere(uint64_t * state)
{
    uint32_t s=cvm_rng_pcg_a(state);
    float z=2.0f*cvm_rng_float_16(&s)-1.0f;
    /// acosf(2*rand-1)=zenith, calculated by probability distribution for surface of a sphere
    /// specifically inverting the integral of the relative probability of each angle over the circle (this probability is sin(angle))
    /// z from this zenith is cos(zenith) which is just 2*rand-1 !
    /// this means that every z coordinate is equally likely !
    float p=sqrtf(1.0f-z*z);
    ///equivalent to sin(zenith)
    float a=cvm_rng_float_16_get(s)*(float)TAU;
    return (vec3f){ .x=p*cosf(a),.y=p*sinf(a),.z=z};
}

static inline vec3f cvm_rng_point_in_ball(uint64_t * state)
{
    uint32_t s=cvm_rng_pcg_a(state);
    float z=2.0f*cvm_rng_float_16(&s)-1.0f;///see notes above
    float p=sqrtf(1.0f-z*z);
    float a=cbrtf(cvm_rng_float_16(&s));///radius
    p*=a;
    z*=a;
    a=cvm_rng_float_16_get(s)*(float)TAU;
    return (vec3f){ .x=p*cosf(a),.y=p*sinf(a),.z=z};
}

static inline vec2f cvm_rng_point_on_circle(uint64_t * state)
{
    float a=(float)TAU*cvm_rng_float_16_get(cvm_rng_pcg_a(state));
    return (vec2f){ .x=cosf(a),.y=sinf(a)};
}

static inline vec2f cvm_rng_point_in_disc(uint64_t * state)
{
    uint32_t s=cvm_rng_pcg_a(state);
    float r=sqrtf(cvm_rng_float_16(&s));
    float a=cvm_rng_float_16_get(s)*(float)TAU;
    return (vec2f){ .x=r*cosf(a),.y=r*sinf(a)};
}

static inline float cvm_rng_float(uint64_t * state)
{
    return cvm_rng_float_16_get(cvm_rng_pcg_a(state));
}

/// effectively takes even distribution over the surface of a sphere and returns the zenith angle
/// can multiply by PI_RCP to get a nice 0-1 distribution that favours the centre
static inline float cvm_rng_even_zenith(uint64_t * state)
{
    return acosf(2.0f*cvm_rng_float_16_get(cvm_rng_pcg_a(state))-1.0f);
}

//#define CVM_INTRINSIC_MODE_NONE
#define CVM_INTRINSIC_MODE_SSE /*this can trick IDE, when it doesn't pick up __SSE__ :p*/

#if (defined __SSE__ || defined CVM_INTRINSIC_MODE_SSE) && !defined CVM_INTRINSIC_MODE_NONE
static inline rotor3f cvm_rng_rotation_3d(uint64_t * state)
{
    uint32_t s=cvm_rng_pcg_a(state);
    float a1=cvm_rng_float_16(&s)*(float)PI;
    float a2=cvm_rng_float_16(&s)*(float)TAU;
    float r1=cvm_rng_float_16_get(s);
    float r2=sqrtf(1.0f-r1);
    r1=sqrtf(r1);

    return (rotor3f){.r=_mm_mul_ps(_mm_set_ps(r2,r2,r1,r1),_mm_set_ps(cosf(a2),sinf(a2),cosf(a1),sinf(a1)))};
}
#else
static inline rotor3f cvm_rng_rotation_3d(uint64_t * state)
{
    /// calculate rotor.s and xy ratio/values based on expected z distribution of resultant z axis (expected to be uniform!)
    ///     ^ this is bacause every axis's z (or x/y) component, after rotation should be a uniformly distributed probability,
    ///         ^ i.e. distribution as seen with for z in cvm_rng_point_on_sphere
    /// r is uniformly random number, 0-1
    /// z = 2r-1 = 1-2*(zx^2 + yz^2) = 2*(s^2+xy^2)-1 (from r3f_get_z_axis)
    /// last step above above uses:  s^2+xy^2+yz^2+zx^2=1  -->  s^2+xy^2-1 = yz^2+zx^2  -->  2*(s^2+xy^2)-2 = -2*(yz^2+zx^2)
    /// A) r = s^2 + xy^2
    /// r = 1 - yz^2 - zx^2
    /// B) 1 - r = yz^2 + zx^2
    /// of note: r is the same as 1-r here because  0-1 is no different to 0-1 for uniform distribution (definition of r)
    /// A and B are effectively formulas for circles, with radii sqrt(r) and sqrt(1-r) respectively
    /// can uniformly distribute directions over these circles quite easily (sin/cos of random angle)
    /// does require assumption that the 2 circles rotations are independent, which should be the case (not 100% certain of this!)
    /// also of note: the selection of which axis (xy,yz,zx) as well as use of r vs 1-r is completely arbitrary, could swap any axis used here with no change in result
    /// ergo how components are assigned here doesn't seem to matter at all
    /// only that radii are paired with the angles for the same circles is all that's important (is even this required ?)

    /// s component should always be positive (effecive explaination is that negative rotation is identical to positive rotation in opposite direction)
    /// easy way to accomplish this is to use PI range and take sine value of that angle

    uint32_t s=cvm_rng_pcg_a(state);
    float a1=cvm_rng_float_16(&s)*(float)PI;
    float a2=cvm_rng_float_16(&s)*(float)TAU;
    float r1=cvm_rng_float_16_get(s);
    float r2=sqrtf(1.0f-r1);
    r1=sqrtf(r1);

    return (rotor3f){.s=r1*sinf(a1),.xy=r1*cosf(a1),.yz=r2*sinf(a2),.zx=r2*cosf(a2)};
}
#endif


#endif




