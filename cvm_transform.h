/**
Copyright 2021 Carl van Mastrigt

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
    cvm_transform_frame stack[VIEW_STACK_SIZE];
    uint32_t i;
}
cvm_transform_stack;


static inline void cvm_transform_stack_reset(cvm_transform_stack * ts)
{
    ts->i=0;
    ts->stack[0].r=(rotor3f){.s=1.0,.xy=0.0f,.yz=0.0f,.zx=0.0f};
    ts->stack[0].p=(vec3f){.x=0.0f,.y=0.0f,.z=0.0f};
}

static inline void cvm_transform_stack_push(cvm_transform_stack * ts)
{
    ts->stack[ts->i+1]=ts->stack[ts->i];
    ts->i++;
}

static inline void cvm_transform_stack_pop(cvm_transform_stack * ts)
{
    ts->i--;
}

static inline void cvm_transform_construct_from_rotor_and_position(float * transform_data,rotor3f r,vec3f p)
{
    ///multiplying by SQRT_2 four times here removes 18 future multiplications by 2
    r.s*=SQRT_2;
    r.xy*=SQRT_2;
    r.yz*=SQRT_2;
    r.zx*=SQRT_2;

    ///essentially just copied from r3f_to_m3f
    ///results stored as row major for more efficient access on device side
    ///     ^just need to declare values as row_major, which ONLY affects how data is READ from, not how matrix access operations are interpreted in shader
    /// written in memory order for better streaming results
    transform_data[0 ]= 1.0 - r.xy*r.xy - r.zx*r.zx;
    transform_data[1 ]= r.yz*r.zx - r.s*r.xy;
    transform_data[2 ]= r.yz*r.xy + r.s*r.zx;
    transform_data[3 ]=p.x;

    transform_data[4 ]= r.zx*r.yz + r.xy*r.s;
    transform_data[5 ]= 1.0 - r.xy*r.xy - r.yz*r.yz;
    transform_data[6 ]= r.zx*r.xy - r.yz*r.s;
    transform_data[7 ]=p.y;

    transform_data[8 ]= r.xy*r.yz - r.zx*r.s;
    transform_data[9 ]= r.xy*r.zx + r.yz*r.s;
    transform_data[10]= 1.0 - r.zx*r.zx - r.yz*r.yz;
    transform_data[11]=p.z;
}

static inline void cvm_transform_stack_get(cvm_transform_stack * ts,float * transform_data)
{
    cvm_transform_construct_from_rotor_and_position(transform_data,ts->stack[ts->i].r,ts->stack[ts->i].p);
}

static inline void cvm_transform_stack_get_scaled(cvm_transform_stack * ts,float * transform_data,float scale)
{
    rotor3f r=ts->stack[ts->i].r;
    vec3f p=ts->stack[ts->i].p;

    r.s*=SQRT_2;
    r.xy*=SQRT_2;
    r.yz*=SQRT_2;
    r.zx*=SQRT_2;

    ///essentially just copied from r3f_to_m3f
    ///results stored as row major for more efficient access on device side
    ///     ^just need to declare values as row_major, which ONLY affects how data is READ from, not how matrix access operations are interpreted in shader
    /// written in memory order for better streaming results
    transform_data[0 ]= scale*(1.0 - r.xy*r.xy -r.zx*r.zx);
    transform_data[1 ]= scale*(r.yz*r.zx - r.s*r.xy);
    transform_data[2 ]= scale*(r.yz*r.xy + r.s*r.zx);
    transform_data[3 ]= p.x;

    transform_data[4 ]= scale*(r.zx*r.yz + r.xy*r.s);
    transform_data[5 ]= scale*(1.0- r.xy*r.xy - r.yz*r.yz);
    transform_data[6 ]= scale*(r.zx*r.xy - r.yz*r.s);
    transform_data[7 ]= p.y;

    transform_data[8 ]= scale*(r.xy*r.yz - r.zx*r.s);
    transform_data[9 ]= scale*(r.xy*r.zx + r.yz*r.s);
    transform_data[10]= scale*(1.0 - r.zx*r.zx - r.yz*r.yz);
    transform_data[11]= p.z;
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
    return r3f_get_y_axis(ts->stack[ts->i].r);;
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
    ts->stack[ts->i].r=r3f_multiply(r3f_from_v3f_and_angle(v,a),ts->stack[ts->i].r);
}

static inline void cvm_transform_stack_rotate(cvm_transform_stack * ts,rotor3f r)
{
    ts->stack[ts->i].r=r3f_multiply(r,ts->stack[ts->i].r);
}

#endif

