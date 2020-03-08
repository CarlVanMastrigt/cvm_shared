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


#ifndef CVM_VIEW
#define CVM_VIEW

typedef struct view_vec3f
{
    GLfloat x;
    GLfloat y;
    GLfloat z;
}
view_vec3f;

typedef struct view_matrix3f ///format is euclidian (starting) axis - entry in transformed axis
{
    view_vec3f x;
    view_vec3f y;
    view_vec3f z;
}
view_matrix3f;

typedef struct view_slice
{
    view_vec3f pos;
    view_matrix3f rot;
}
view_slice;


typedef struct view
{
    quaternion q;
    vec3f p;
}
view;

#define VIEW_STACK_SIZE 256

typedef struct view_stack
{
    view stack[VIEW_STACK_SIZE];
    uint32_t curr;
}
view_stack;


static inline void reset_view_stack(view_stack * vs)
{
    vs->curr=0;
    vs->stack[0].q=(quaternion){.r=1.0,.x=0.0f,.y=0.0f,.z=0.0f};
    vs->stack[0].p=(vec3f){.x=0.0f,.y=0.0f,.z=0.0f};
}

static inline void shift_plus_view_stack(view_stack * vs)
{
    vs->stack[vs->curr+1]=vs->stack[vs->curr];
    vs->curr++;
}
static inline void shift_minus_view_stack(view_stack * vs)
{
    vs->curr--;
}

static inline view_slice get_current_slice(view_stack * vs)
{
    view_slice s;
    quaternion q=vs->stack[vs->curr].q;
    vec3f p=vs->stack[vs->curr].p;

    q.r*=SQRT_2;
    q.x*=SQRT_2;
    q.y*=SQRT_2;
    q.z*=SQRT_2;

    s.rot.x.x=1.0 - q.y*q.y - q.z*q.z;
    s.rot.x.y=q.x*q.y + q.r*q.z;
    s.rot.x.z=q.x*q.z - q.r*q.y;
    s.rot.y.x=q.x*q.y - q.r*q.z;
    s.rot.y.y=1.0 - q.x*q.x - q.z*q.z;
    s.rot.y.z=q.y*q.z + q.r*q.x;
    s.rot.z.x=q.x*q.z + q.r*q.y;
    s.rot.z.y=q.y*q.z - q.r*q.x;
    s.rot.z.z=1.0 - q.x*q.x - q.y*q.y;

    s.pos=(view_vec3f){.x=p.x,.y=p.y,.z=p.z};

    return s;
}

static inline view_slice get_current_slice_scaled(view_stack * vs,float scale)
{
    view_slice s;
    quaternion q=vs->stack[vs->curr].q;
    vec3f p=vs->stack[vs->curr].p;

    q.r*=SQRT_2;
    q.x*=SQRT_2;
    q.y*=SQRT_2;
    q.z*=SQRT_2;

    s.rot.x.x=scale*(1.0 - q.y*q.y - q.z*q.z);
    s.rot.x.y=scale*(q.x*q.y + q.r*q.z);
    s.rot.x.z=scale*(q.x*q.z - q.r*q.y);
    s.rot.y.x=scale*(q.x*q.y - q.r*q.z);
    s.rot.y.y=scale*(1.0 - q.x*q.x - q.z*q.z);
    s.rot.y.z=scale*(q.y*q.z + q.r*q.x);
    s.rot.z.x=scale*(q.x*q.z + q.r*q.y);
    s.rot.z.y=scale*(q.y*q.z - q.r*q.x);
    s.rot.z.z=scale*(1.0 - q.x*q.x - q.y*q.y);

    s.pos=(view_vec3f){.x=p.x,.y=p.y,.z=p.z};

    return s;
}

static inline view_vec3f get_modified_position(view_stack * vs,vec3f pos)
{
    pos=v3f_add(vs->stack[vs->curr].p,rotate_vector_by_quaternion(vs->stack[vs->curr].q,pos));
    return (view_vec3f){.x=pos.x,.y=pos.y,.z=pos.z};
}

static inline view_vec3f get_current_slice_pos(view_stack * vs)
{
    vec3f p=vs->stack[vs->curr].p;
    return (view_vec3f){.x=p.x,.y=p.y,.z=p.z};
}
static inline view_vec3f get_current_slice_x_axis(view_stack * vs)
{
    vec3f p=quaternion_get_x_axis(vs->stack[vs->curr].q);
    return (view_vec3f){.x=p.x,.y=p.y,.z=p.z};
}
static inline view_vec3f get_current_slice_y_axis(view_stack * vs)
{
    vec3f p=quaternion_get_y_axis(vs->stack[vs->curr].q);
    return (view_vec3f){.x=p.x,.y=p.y,.z=p.z};
}
static inline view_vec3f get_current_slice_z_axis(view_stack * vs)
{
    vec3f p=quaternion_get_z_axis(vs->stack[vs->curr].q);
    return (view_vec3f){.x=p.x,.y=p.y,.z=p.z};
}

static inline void position_view(view_stack * vs,vec3f pos)
{
    vs->stack[vs->curr].p=v3f_add(vs->stack[vs->curr].p,rotate_vector_by_quaternion(vs->stack[vs->curr].q,pos));
}
static inline void rotate_view_x(view_stack * vs,float angle)
{
    vs->stack[vs->curr].q=quat_rot_x_axis(vs->stack[vs->curr].q,angle);
}
static inline void rotate_view_y(view_stack * vs,float angle)
{
    vs->stack[vs->curr].q=quat_rot_y_axis(vs->stack[vs->curr].q,angle);
}
static inline void rotate_view_z(view_stack * vs,float angle)
{
    vs->stack[vs->curr].q=quat_rot_z_axis(vs->stack[vs->curr].q,angle);
}
static inline void rotate_view_vector(view_stack * vs,vec3f v,float a)/// MUST BE UNIT VECTOR
{
    vs->stack[vs->curr].q=quaternion_multiply(vs->stack[vs->curr].q,get_quaternion_from_vector(v,a));
}
static inline void rotate_view_quaternion(view_stack * vs,quaternion q)/// MUST BE UNIT QUATERNION
{
    vs->stack[vs->curr].q=quaternion_multiply(vs->stack[vs->curr].q,q);
}

#endif
