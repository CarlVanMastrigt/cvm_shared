/**
Copyright 2021,2022,2023 Carl van Mastrigt

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


#ifndef CVM_TRANSFORM
#define CVM_TRANSFORM


///is it worth it to store rotation matrix and bool saying it has been calculated? can potentially save 18 mults and 12 adds
typedef struct cvm_transform_frame
{
    rotor3f r;
    vec3f p;
}
cvm_transform_frame;

#define CVM_TRANSFORM_STACK_SIZE 256

typedef struct cvm_transform_stack
{
    cvm_transform_frame stack[CVM_TRANSFORM_STACK_SIZE];
    uint32_t i;
}
cvm_transform_stack;


static inline void cvm_transform_stack_reset(cvm_transform_stack * ts)
{
    ts->i=0;
    ts->stack[0].r=r3f_ini(1.0f,0.0f,0.0f,0.0f);
    ts->stack[0].p=v3f_ini(0.0f,0.0f,0.0f);
}

static inline void cvm_transform_stack_push(cvm_transform_stack * ts)
{
    ts->stack[ts->i+1]=ts->stack[ts->i];
    ts->i++;
    assert(ts->i < CVM_TRANSFORM_STACK_SIZE);
}

static inline void cvm_transform_stack_pop(cvm_transform_stack * ts)
{
    ts->i--;
}

//#define CVM_INTRINSIC_MODE_NONE
//#define CVM_INTRINSIC_MODE_SSE /*this can trick IDE, when it doesn't pick up __SSE__ :p*/

#if (defined __SSE__ || defined CVM_INTRINSIC_MODE_SSE) && !defined CVM_INTRINSIC_MODE_NONE
static inline void cvm_transform_construct_from_rotor_and_position(float * transform_data,rotor3f r,vec3f p)
{
    __m128 r2=_mm_mul_ps(r.r,_mm_set1_ps(SQRT_2));

    __m128 xy_yz_zx=_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r2),0x39));/// xy,yz,zx,s
    __m128 zx_xy_yz=_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r2),0x27));/// zx,xy,yz,s
    __m128 yz_zx_xy=_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r2),0x1E));/// yz,zx,xy,s
    __m128 s3=_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r2),0x00));/// s,s,s,s

    __m128 sqred=_mm_sub_ps(_mm_sub_ps(_mm_set1_ps(1.0f),_mm_mul_ps(xy_yz_zx,xy_yz_zx)),_mm_mul_ps(zx_xy_yz,zx_xy_yz));
    __m128 added=_mm_add_ps(_mm_mul_ps(zx_xy_yz,yz_zx_xy),_mm_mul_ps(xy_yz_zx,s3));
    __m128 subed=_mm_sub_ps(_mm_mul_ps(xy_yz_zx,yz_zx_xy),_mm_mul_ps(zx_xy_yz,s3));

    _mm_store_ps(transform_data+0,_mm_blend_ps(_mm_blend_ps(sqred,subed,0x02),_mm_blend_ps(added,_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(p.v),0x00)),0x08),0x0C));
    _mm_store_ps(transform_data+4,_mm_blend_ps(_mm_blend_ps(sqred,subed,0x04),_mm_blend_ps(added,_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(p.v),0x55)),0x08),0x09));
    _mm_store_ps(transform_data+8,_mm_blend_ps(_mm_blend_ps(sqred,subed,0x01),_mm_blend_ps(added,_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(p.v),0xAA)),0x08),0x0A));
}
static inline void cvm_transform_construct_from_rotor_and_position_scaled(float * transform_data,rotor3f r,vec3f p,float scale)
{
    __m128 r2=_mm_mul_ps(r.r,_mm_set1_ps(SQRT_2));

    __m128 scale_v=_mm_set1_ps(scale);
    __m128 xy_yz_zx=_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r2),0x39));/// xy,yz,zx,s
    __m128 zx_xy_yz=_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r2),0x27));/// zx,xy,yz,s
    __m128 yz_zx_xy=_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r2),0x1E));/// yz,zx,xy,s
    __m128 s3=_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r2),0x00));/// s,s,s,s

    __m128 sqred=_mm_mul_ps(scale_v,_mm_sub_ps(_mm_sub_ps(_mm_set1_ps(1.0f),_mm_mul_ps(xy_yz_zx,xy_yz_zx)),_mm_mul_ps(zx_xy_yz,zx_xy_yz)));
    __m128 added=_mm_mul_ps(scale_v,_mm_add_ps(_mm_mul_ps(zx_xy_yz,yz_zx_xy),_mm_mul_ps(xy_yz_zx,s3)));
    __m128 subed=_mm_mul_ps(scale_v,_mm_sub_ps(_mm_mul_ps(xy_yz_zx,yz_zx_xy),_mm_mul_ps(zx_xy_yz,s3)));


    _mm_store_ps(transform_data+0,_mm_blend_ps(_mm_blend_ps(sqred,subed,0x02),_mm_blend_ps(added,_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(p.v),0x00)),0x08),0x0C));
    _mm_store_ps(transform_data+4,_mm_blend_ps(_mm_blend_ps(sqred,subed,0x04),_mm_blend_ps(added,_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(p.v),0x55)),0x08),0x09));
    _mm_store_ps(transform_data+8,_mm_blend_ps(_mm_blend_ps(sqred,subed,0x01),_mm_blend_ps(added,_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(p.v),0xAA)),0x08),0x0A));
}
#else
static inline void cvm_transform_construct_from_rotor_and_position(float * transform_data,rotor3f r,vec3f p)
{
    assert((((uintptr_t)transform_data)&0xF)==0);

    r.s*=(float)SQRT_2;
    r.xy*=(float)SQRT_2;
    r.yz*=(float)SQRT_2;
    r.zx*=(float)SQRT_2;

    ///essentially just copied from r3f_to_m3f
    ///results stored as row major for more efficient access on device side
    ///     ^just need to declare values as row_major, which ONLY affects how data is READ from, not how matrix access operations are interpreted in shader
    /// written in memory order for better streaming results
    transform_data[0 ]= 1.0f - r.xy*r.xy - r.zx*r.zx;
    transform_data[1 ]= r.yz*r.zx - r.s*r.xy;
    transform_data[2 ]= r.yz*r.xy + r.s*r.zx;
    transform_data[3 ]=p.x;

    transform_data[4 ]= r.zx*r.yz + r.xy*r.s;
    transform_data[5 ]= 1.0f - r.xy*r.xy - r.yz*r.yz;
    transform_data[6 ]= r.zx*r.xy - r.yz*r.s;
    transform_data[7 ]=p.y;

    transform_data[8 ]= r.xy*r.yz - r.zx*r.s;
    transform_data[9 ]= r.xy*r.zx + r.yz*r.s;
    transform_data[10]= 1.0f - r.zx*r.zx - r.yz*r.yz;
    transform_data[11]=p.z;
}
static inline void cvm_transform_construct_from_rotor_and_position_scaled(float * transform_data,rotor3f r,vec3f p,float scale)
{
    assert((((uintptr_t)transform_data)&0xF)==0);

    r.s*=(float)SQRT_2;
    r.xy*=(float)SQRT_2;
    r.yz*=(float)SQRT_2;
    r.zx*=(float)SQRT_2;

    ///essentially just copied from r3f_to_m3f
    ///results stored as row major for more efficient access on device side
    ///     ^just need to declare values as row_major, which ONLY affects how data is READ from, not how matrix access operations are interpreted in shader
    /// written in memory order for better streaming results
    transform_data[0 ]= scale*(1.0f - r.xy*r.xy -r.zx*r.zx);
    transform_data[1 ]= scale*(r.yz*r.zx - r.s*r.xy);
    transform_data[2 ]= scale*(r.yz*r.xy + r.s*r.zx);
    transform_data[3 ]= p.x;

    transform_data[4 ]= scale*(r.zx*r.yz + r.xy*r.s);
    transform_data[5 ]= scale*(1.0f- r.xy*r.xy - r.yz*r.yz);
    transform_data[6 ]= scale*(r.zx*r.xy - r.yz*r.s);
    transform_data[7 ]= p.y;

    transform_data[8 ]= scale*(r.xy*r.yz - r.zx*r.s);
    transform_data[9 ]= scale*(r.xy*r.zx + r.yz*r.s);
    transform_data[10]= scale*(1.0f - r.zx*r.zx - r.yz*r.yz);
    transform_data[11]= p.z;
}
#endif

static inline void cvm_transform_stack_get(cvm_transform_stack * ts,float * transform_data)
{
    cvm_transform_construct_from_rotor_and_position(transform_data,ts->stack[ts->i].r,ts->stack[ts->i].p);
}

static inline void cvm_transform_stack_get_scaled(cvm_transform_stack * ts,float * transform_data,float scale)
{
    cvm_transform_construct_from_rotor_and_position_scaled(transform_data,ts->stack[ts->i].r,ts->stack[ts->i].p,scale);
}

static inline vec3f cvm_transform_stack_get_transformed_position(cvm_transform_stack * ts,vec3f p)
{
    return v3f_add(ts->stack[ts->i].p,r3f_rotate_v3f(ts->stack[ts->i].r,p));
}

static inline vec3f cvm_transform_stack_get_position(cvm_transform_stack * ts)
{
    return ts->stack[ts->i].p;
}

static inline vec3f cvm_transform_stack_get_x_axis(cvm_transform_stack * ts)
{
    return r3f_get_x_axis(ts->stack[ts->i].r);
}

static inline vec3f cvm_transform_stack_get_y_axis(cvm_transform_stack * ts)
{
    return r3f_get_y_axis(ts->stack[ts->i].r);
}

static inline vec3f cvm_transform_stack_get_z_axis(cvm_transform_stack * ts)
{
    return r3f_get_z_axis(ts->stack[ts->i].r);
}

static inline void cvm_transform_stack_offset_position(cvm_transform_stack * ts,vec3f offset)
{
    ts->stack[ts->i].p=v3f_add(ts->stack[ts->i].p,r3f_rotate_v3f(ts->stack[ts->i].r,offset));
}

static inline void cvm_transform_stack_rotate_around_x_axis(cvm_transform_stack * ts,float a)
{
    ts->stack[ts->i].r=r3f_rotate_around_x_axis(ts->stack[ts->i].r,a);
}

static inline void cvm_transform_stack_rotate_around_y_axis(cvm_transform_stack * ts,float a)
{
    ts->stack[ts->i].r=r3f_rotate_around_y_axis(ts->stack[ts->i].r,a);
}

static inline void cvm_transform_stack_rotate_around_z_axis(cvm_transform_stack * ts,float a)
{
    ts->stack[ts->i].r=r3f_rotate_around_z_axis(ts->stack[ts->i].r,a);
}

static inline void cvm_transform_stack_rotate_around_vector(cvm_transform_stack * ts,vec3f v,float a)
{
    ts->stack[ts->i].r=r3f_mul(r3f_from_angle(v,a),ts->stack[ts->i].r);
}

static inline void cvm_transform_stack_rotate(cvm_transform_stack * ts,rotor3f r)
{
    ts->stack[ts->i].r=r3f_mul(r,ts->stack[ts->i].r);
}

#endif

