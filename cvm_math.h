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

#ifndef CVM_MATH_H
#define CVM_MATH_H

#ifndef PI
#define PI 3.14159265358979
#endif

#ifndef SQRT_3
#define SQRT_3 1.7320508075688
#endif

#ifndef SQRT_2
#define SQRT_2 1.414213562373
#endif

#ifndef SQRT_HALF
#define SQRT_HALF 0.70710678118654
#endif



typedef struct vec3i
{
    int32_t x;
    int32_t y;
    int32_t z;
}
vec3i;

typedef struct vec2i
{
    int32_t x;
    int32_t y;
}
vec2i;

typedef struct vec4f
{
    float x;
    float y;
    float z;
    float w;
}
vec4f;

typedef struct vec3f
{
    float x;
    float y;
    float z;
}
vec3f;

typedef struct vec2f
{
    float x;
    float y;
}
vec2f;

typedef struct rectangle
{
    int x;
    int y;
    int w;
    int h;
}
rectangle;

typedef struct quaternion
{
    float r;

    float x;
    float y;
    float z;
}
quaternion;

typedef struct plane
{
    vec3f n;///plane normal
    float d;///offset of plane along normal(n) / distance of plane from origin
}
plane;


///for matrices vector x,y,z,w represents the vector that those respective euclidean basis vector map to
typedef struct matrix4f
{
    vec4f x;
    vec4f y;
    vec4f z;
    vec4f w;
}
matrix4f;

typedef struct matrix3f
{
    vec3f x;
    vec3f y;
    vec3f z;
}
matrix3f;

typedef struct matrix2f
{
    vec2f x;
    vec2f y;
}
matrix2f;











///conversion
static inline vec3f v3i_to_v3f(vec3i v)
{
    return (vec3f){.x=(float)v.x,.y=(float)v.y,.z=(float)v.z};
}
static inline vec2f v2i_to_v2f(vec2i v)
{
    return (vec2f){.x=(float)v.x,.y=(float)v.y};
}
static inline vec2f v2f_modf(vec2f v,vec2i * i)
{
    v.x-=(float)(i->x=(int32_t)v.x);
    v.y-=(float)(i->y=(int32_t)v.y);
    return v;
}
static inline vec2i v2f_to_v2i(vec2f v)
{
    return (vec2i){(int32_t)v.x,(int32_t)v.y};
}




///vec3i
static inline vec3i v3i_add(vec3i v1,vec3i v2)
{
    return (vec3i){.x=v1.x+v2.x,.y=v1.y+v2.y,.z=v1.z+v2.z};
}
static inline vec3i v3i_sub(vec3i v1,vec3i v2)
{
    return (vec3i){.x=v1.x-v2.x,.y=v1.y-v2.y,.z=v1.z-v2.z};
}
static inline vec3i v3i_mult(vec3i v,int32_t c)
{
    return (vec3i){.x=v.x*c,.y=v.y*c,.z=v.z*c};
}
static inline vec3i v3i_div(vec3i v,int32_t c)
{
    return (vec3i){.x=v.x/c,.y=v.y/c,.z=v.z/c};
}
static inline vec3i v3i_mod(vec3i v,int32_t m)
{
    return (vec3i){.x=v.x%m,.y=v.y%m,.z=v.z%m};
}

///vec2i
static inline vec2i v2i_add(vec2i v1,vec2i v2)
{
    return (vec2i){.x=v1.x+v2.x,.y=v1.y+v2.y};
}
static inline vec2i v2i_sub(vec2i v1,vec2i v2)
{
    return (vec2i){.x=v1.x-v2.x,.y=v1.y-v2.y};
}
static inline vec2i v2i_mult(vec2i v,int32_t c)
{
    return (vec2i){.x=v.x*c,.y=v.y*c};
}
static inline vec2i v2i_div(vec2i v,int32_t c)
{
    return (vec2i){.x=v.x/c,.y=v.y/c};
}
static inline vec2i v2i_mod(vec2i v,int32_t m)
{
    return (vec2i){.x=v.x%m,.y=v.y%m};
}
static inline int32_t v2i_dot(vec2i v1,vec2i v2)
{
    return (v1.x*v2.x)+(v1.y*v2.y);
}
static inline int32_t v2i_mag_sq(vec2i v)
{
    return v.x*v.x+v.y*v.y;
}
static inline int32_t v2i_dist_sq(vec2i v1,vec2i v2)
{
    return (v1.x-v2.x)*(v1.x-v2.x)+(v1.y-v2.y)*(v1.y-v2.y);
}
static inline vec2i v2i_orth(vec2i v)/// PI/2 rotation
{
    return (vec2i){.x= -v.y,.y= v.x};
}
static inline vec2i v2i_ls(vec2i v,int32_t left_shift)
{
    return (vec2i){.x= v.x<<left_shift,.y= v.y<<left_shift};
}
static inline vec2i v2i_rs(vec2i v,int32_t right_shift)
{
    return (vec2i){.x= v.x>>right_shift,.y= v.y>>right_shift};
}
static inline vec2i v2i_min(vec2i v1,vec2i v2)
{
    return (vec2i){ .x= v1.x+(((v2.x-v1.x)>>31)&(v2.x-v1.x)),
                    .y= v1.y+(((v2.y-v1.y)>>31)&(v2.y-v1.y))};
}
static inline vec2i v2i_max(vec2i v1,vec2i v2)
{
    return (vec2i){ .x= v1.x-(((v1.x-v2.x)>>31)&(v1.x-v2.x)),
                    .y= v1.y-(((v1.y-v2.y)>>31)&(v1.y-v2.y))};
}
static inline vec2i v2i_clamp(vec2i v,vec2i min,vec2i max)
{
    return (vec2i){ .x= v.x-(((v.x-min.x)>>31)&(v.x-min.x))+(((max.x-v.x)>>31)&(max.x-v.x)),
                    .y= v.y-(((v.y-min.y)>>31)&(v.y-min.y))+(((max.y-v.y)>>31)&(max.y-v.y))};
}







///vec3f
static inline vec3f v3f_add(vec3f v1,vec3f v2)
{
    return (vec3f){.x=v1.x+v2.x,.y=v1.y+v2.y,.z=v1.z+v2.z};
}
static inline vec3f v3f_sub(vec3f v1,vec3f v2)
{
    return (vec3f){.x=v1.x-v2.x,.y=v1.y-v2.y,.z=v1.z-v2.z};
}
static inline vec3f v3f_mult(vec3f v,float c)
{
    return (vec3f){.x=v.x*c,.y=v.y*c,.z=v.z*c};
}
static inline vec3f v3f_div(vec3f v,float c)
{
    return (vec3f){.x=v.x/c,.y=v.y/c,.z=v.z/c};
}
static inline vec3f v3f_norm(vec3f v)
{
    float inv_mag=1.0f/sqrtf(v.x*v.x+v.y*v.y+v.z*v.z);
    return (vec3f){.x=v.x*inv_mag,.y=v.y*inv_mag,.z=v.z*inv_mag};
}
static inline float v3f_mag(vec3f v)
{
    return sqrt(v.x*v.x+v.y*v.y+v.z*v.z);
}
static inline float v3f_dist(vec3f v1,vec3f v2)
{
    return v3f_mag(v3f_sub(v1,v2));
}
static inline float v3f_mag_sq(vec3f v)
{
    return v.x*v.x+v.y*v.y+v.z*v.z;
}
static inline float v3f_dist_sq(vec3f v1,vec3f v2)
{
    return v3f_mag_sq(v3f_sub(v1,v2));
}
static inline float v3f_dot(vec3f v1,vec3f v2)
{
    return v1.x*v2.x+v1.y*v2.y+v1.z*v2.z;
}
static inline vec3f v3f_cross(vec3f v1,vec3f v2)
{
    return (vec3f){.x=v1.y*v2.z - v1.z*v2.y,.y=v1.z*v2.x - v1.x*v2.z,.z=v1.x*v2.y - v1.y*v2.x};
}
static inline vec3f v3f_norm_cross(vec3f v1,vec3f v2)
{
    return v3f_norm(v3f_cross(v1,v2));
}
static inline vec3f v3f_mid(vec3f v1,vec3f v2)
{
    return (vec3f){.x=(v1.x+v2.x)*0.5f,.y=(v1.y+v2.y)*0.5f,.z=(v1.z+v2.z)*0.5f};
}
static inline vec3f v3f_norm_mid(vec3f v1,vec3f v2)
{
    return v3f_norm(v3f_mid(v1,v2));
}
static inline vec3f v3f_rotate(vec3f v,vec3f k,float cos_theta,float sin_theta)///Rodrigues rotation
{
    float d=(k.x*v.x+k.y*v.y+k.z*v.z)*(1.0f-cos_theta);

    return (vec3f){ .x= v.x*cos_theta  +  (k.y*v.z - k.z*v.y)*sin_theta  +  k.x*d,
                    .y= v.y*cos_theta  +  (k.z*v.x - k.x*v.z)*sin_theta  +  k.y*d,
                    .z= v.z*cos_theta  +  (k.x*v.y - k.y*v.x)*sin_theta  +  k.z*d};
}
static inline vec3f v3f_from_spherical(float r,float zenith,float azimuth)
{
    return (vec3f){ .x=r*cosf(azimuth)*sinf(zenith),
                    .y=r*sinf(azimuth)*sinf(zenith),
                    .z=r*cosf(zenith)};
}
static inline vec3f v3f_max(vec3f v1,vec3f v2)
{
    return (vec3f){.x=(v1.x>v2.x)?v1.x:v2.x,.y=(v1.y>v2.y)?v1.y:v2.y,.z=(v1.z>v2.z)?v1.z:v2.z};
}
static inline vec3f v3f_min(vec3f v1,vec3f v2)
{
    return (vec3f){.x=(v1.x<v2.x)?v1.x:v2.x,.y=(v1.y<v2.y)?v1.y:v2.y,.z=(v1.z<v2.z)?v1.z:v2.z};
}
static inline vec3f v3f_clamp_range(vec3f v,float min,float max)
{
    return (vec3f){ .x=(v.x<min)?min:(v.x>max)?max:v.x,
                    .y=(v.y<min)?min:(v.y>max)?max:v.y,
                    .z=(v.z<min)?min:(v.z>max)?max:v.z};
}




///vec2f
static inline vec2f v2f_add(vec2f v1,vec2f v2)
{
    return (vec2f){.x=v1.x+v2.x,.y=v1.y+v2.y};
}
static inline vec2f v2f_sub(vec2f v1,vec2f v2)
{
    return (vec2f){.x=v1.x-v2.x,.y=v1.y-v2.y};
}
static inline vec2f v2f_mult(vec2f v,float c)
{
    return (vec2f){.x=v.x*c,.y=v.y*c};
}
static inline vec2f v2f_div(vec2f v,float c)
{
    return (vec2f){.x=v.x/c,.y=v.y/c};
}
static inline vec2f v2f_norm(vec2f v)
{
    float inv_mag=1.0f/sqrtf(v.x*v.x+v.y*v.y);
    return (vec2f){.x=v.x*inv_mag,.y=v.y*inv_mag};
}
static inline float v2f_mag(vec2f v)
{
    return sqrt(v.x*v.x+v.y*v.y);
}
static inline float v2f_dist(vec2f v1,vec2f v2)
{
    return v2f_mag(v2f_sub(v1,v2));
}
static inline float v2f_mag_sq(vec2f v)
{
    return v.x*v.x+v.y*v.y;
}
static inline float v2f_dist_sq(vec2f v1,vec2f v2)
{
    return v2f_mag_sq(v2f_sub(v1,v2));
}
static inline float v2f_dot(vec2f v1,vec2f v2)
{
    return v1.x*v2.x+v1.y*v2.y;
}
static inline vec2f v2f_min(vec2f v1,vec2f v2)
{
    return (vec2f){.x=(v1.x<v2.x)?v1.x:v2.x,.y=(v1.y<v2.y)?v1.y:v2.y};
}
static inline vec2f v2f_max(vec2f v1,vec2f v2)
{
    return (vec2f){.x=(v1.x>v2.x)?v1.x:v2.x,.y=(v1.y>v2.y)?v1.y:v2.y};
}
static inline vec2f v2f_clamp(vec2f v,vec2f min,vec2f max)
{
    return (vec2f){.x=(v.x<min.x)?min.x:(v.x>max.x)?max.x:v.x,.y=(v.y<min.y)?min.y:(v.y>max.y)?max.y:v.y};
}
static inline vec2f v2f_from_angle(float angle)
{
    return (vec2f){.x=cosf(angle),.y=sinf(angle)};
}
static inline float v2f_angle(vec2f v)
{
    return atan2f(v.y,v.x);
}
static inline vec2f v2f_rotate(vec2f v,float angle)
{
    float c=cosf(angle),s=sinf(angle);
    return (vec2f){.x=c*v.x-s*v.y,.y=s*v.x+c*v.y};
}
static inline vec2f v2f_orth(vec2f v)/// PI/2 rotation
{
    return (vec2f){.x=-v.y,.y=v.x};
}
static inline vec2f v2f_mid(vec2f v1,vec2f v2)
{
    return (vec2f){.x=(v1.x+v2.x)*0.5f,.y=(v1.y+v2.y)*0.5f};
}





matrix4f m4f_mult(matrix4f l,matrix4f r);
matrix4f m4f_inv(matrix4f m);
vec3f m4f_v4f_mult_p(matrix4f m,vec4f v);


matrix3f m3f_inv(matrix3f m);
matrix3f m3f_inv_det(matrix3f m,float * determinant);
vec3f m3f_v3f_mult(matrix3f m,vec3f v);
matrix3f m3f_mult(matrix3f l,matrix3f r);
static inline float m3f_det(matrix3f m)
{
    return m.x.x*m.y.y*m.z.z + m.y.x*m.z.y*m.x.z + m.z.x*m.x.y*m.y.z - m.z.x*m.y.y*m.x.z - m.y.x*m.x.y*m.z.z - m.x.x*m.z.y*m.y.z;
}



static inline vec4f vec4f_blend(vec4f b,vec4f f)
{
    return (vec4f){.x=(1.0-f.w)*b.x+f.w*f.x,.y=(1.0-f.w)*b.y+f.w*f.y,.z=(1.0-f.w)*b.z+f.w*f.z,.w=(1.0-f.w)*b.w+f.w};
}







bool get_rectangle_overlap(rectangle * rect,rectangle b);

static inline bool rectangle_surrounds_origin(rectangle r)
{
    return ((r.x <= 0)&&(r.y <= 0)&&(r.x+r.w > 0)&&(r.y+r.h > 0));
}

static inline bool rectangle_surrounds_point(rectangle r,vec2i p)
{
    return ((r.x <= p.x)&&(r.y <= p.y)&&(r.x+r.w > p.x)&&(r.y+r.h > p.y));
}

static inline bool rectangles_overlap(rectangle r1,rectangle r2)
{
    return ((r1.x+r1.w > r2.x)&&(r1.y+r1.h > r2.y)&&(r1.x < r2.x+r2.w)&&(r1.y < r2.y+r2.h));
}




static inline quaternion quaternion_multiply(quaternion l,quaternion r)
{
    return (quaternion){.r=l.r*r.r-l.x*r.x-l.y*r.y-l.z*r.z,
                        .x=l.r*r.x+l.x*r.r+l.y*r.z-l.z*r.y,
                        .y=l.r*r.y-l.x*r.z+l.y*r.r+l.z*r.x,
                        .z=l.r*r.z+l.x*r.y-l.y*r.x+l.z*r.r};
}

static inline vec3f quaternion_get_x_axis(quaternion q)
{
    return (vec3f){.x=1.0-2.0*(q.y*q.y+q.z*q.z),.y=2.0*(q.x*q.y+q.r*q.z),.z=2.0*(q.x*q.z-q.r*q.y)};
}
static inline vec3f quaternion_get_y_axis(quaternion q)
{
    return (vec3f){.x=2.0*(q.x*q.y-q.r*q.z),.y=1.0-2.0*(q.x*q.x+q.z*q.z),.z=2.0*(q.y*q.z+q.r*q.x)};
}
static inline vec3f quaternion_get_z_axis(quaternion q)
{
    return (vec3f){.x=2.0*(q.x*q.z+q.r*q.y),.y=2.0*(q.y*q.z-q.r*q.x),.z=1.0-2.0*(q.x*q.x+q.y*q.y)};
}
static inline quaternion get_quaternion_from_vector(vec3f v,float angle)
{
    float s=sinf(angle*0.5);
    return (quaternion){.r=cosf(angle*0.5),.x=v.x*s,.y=v.y*s,.z=v.z*s};
}
static inline vec3f rotate_vector_by_quaternion(quaternion q,vec3f v)
{
    return (vec3f)
    {
        .x=2.0f*((0.5f-q.y*q.y-q.z*q.z)*v.x + (q.x*q.y-q.r*q.z)*v.y + (q.x*q.z+q.r*q.y)*v.z),
        .y=2.0f*((q.x*q.y+q.r*q.z)*v.x + (0.5f-q.x*q.x-q.z*q.z)*v.y + (q.y*q.z-q.r*q.x)*v.z),
        .z=2.0f*((q.x*q.z-q.r*q.y)*v.x + (q.y*q.z+q.r*q.x)*v.y + (0.5f-q.x*q.x-q.y*q.y)*v.z)
    };
}
static inline vec3f rotate_vector_by_quaternion_inverse(quaternion q,vec3f v)
{
    return (vec3f)
    {
        .x=2.0f*((0.5f-q.y*q.y-q.z*q.z)*v.x + (q.x*q.y+q.r*q.z)*v.y + (q.x*q.z-q.r*q.y)*v.z),
        .y=2.0f*((q.x*q.y-q.r*q.z)*v.x + (0.5f-q.x*q.x-q.z*q.z)*v.y + (q.y*q.z+q.r*q.x)*v.z),
        .z=2.0f*((q.x*q.z+q.r*q.y)*v.x + (q.y*q.z-q.r*q.x)*v.y + (0.5f-q.x*q.x-q.y*q.y)*v.z)
    };
}
static inline quaternion quat_rot_x_axis(quaternion q,float angle)
{
    float s=sinf(angle*0.5);
    float c=cosf(angle*0.5);
    return (quaternion){ .r=q.r*c-q.x*s , .x=q.r*s+q.x*c , .y=q.y*c+q.z*s , .z=q.z*c-q.y*s };
}
static inline quaternion quat_rot_y_axis(quaternion q,float angle)
{
    float s=sinf(angle*0.5);
    float c=cosf(angle*0.5);
    return (quaternion){ .r=q.r*c-q.y*s , .x=q.x*c-q.z*s , .y=q.r*s+q.y*c , .z=q.x*s+q.z*c };
}
static inline quaternion quat_rot_z_axis(quaternion q,float angle)
{
    float s=sinf(angle*0.5);
    float c=cosf(angle*0.5);
    return (quaternion){ .r=q.r*c-q.z*s , .x=q.x*c+q.y*s , .y=q.y*c-q.x*s , .z=q.r*s+q.z*c };
}





static inline vec2f m2f_vec2f_multiply(matrix2f m,vec2f v)
{
    return (vec2f){.x=v.x*m.x.x+v.y*m.y.x,.y=v.x*m.x.y+v.y*m.y.y};
}

static inline vec2f vec2f_m2f_multiply(matrix2f m,vec2f v)
{
    return (vec2f){.x=v.x*m.x.x+v.y*m.x.y,.y=v.x*m.y.x+v.y*m.y.y};
}

static inline matrix2f m2f_rotation_matrix(float angle)
{
    matrix2f m;
    m.x.x=m.y.y=cosf(angle);
    m.x.y= -(m.y.x=sinf(angle));
    return m;
}


static inline plane plane_from_normal_and_point(vec3f n,vec3f p)
{
    return (plane){.n=n,.d=v3f_dot(n,p)};
}

static inline float plane_point_dist(plane p,vec3f point)
{
    return v3f_dot(p.n,point)-p.d;
}

#endif

