/**
Copyright 2020,2021 Carl van Mastrigt

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

#ifndef TAU
#define TAU         6.2831853071795864769252867665590057683943387987502116419498891846
#endif

#ifndef PI
#define PI          3.1415926535897932384626433832795028841971693993751058209749445923
#endif

#ifndef PI_RCP
#define PI_RCP      0.3183098861837906715377675267450287240689192914809128974953346881
#endif

#ifndef EULER
#define EULER       2.7182818284590452353602874713526624977572470936999595749669676277
#endif

#ifndef SQRT_3
#define SQRT_3      1.7320508075688772935274463415058723669428052538103806280558069794
#endif

#ifndef SQRT_2
#define SQRT_2      1.4142135623730950488016887242096980785696718753769480731766797379
#endif

#ifndef SQRT_HALF
#define SQRT_HALF   0.7071067811865475244008443621048490392848359376884740365883398689
#endif

#ifndef SQRT_THIRD
#define SQRT_THIRD  0.5773502691896257645091487805019574556476017512701268760186023264
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
    int x1;
    int y1;
    int x2;
    int y2;
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

///not sure bivectors are necessary yet.
typedef struct bivec3f
{
    float xy;
    float yz;
    float zx;
}
bivec3f;

typedef struct rotor3f
{
    float s;///scalar / symmetric component (dot product in ge0metric product)
    float xy;
    float yz;
    float zx;
}
rotor3f;

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
    return (vec2i){ .x= (v1.x<v2.x) ? v1.x : v2.x,
                    .y= (v1.y<v2.y) ? v1.y : v2.y };
}
static inline vec2i v2i_max(vec2i v1,vec2i v2)
{
    return (vec2i){ .x= (v1.x>v2.x) ? v1.x : v2.x,
                    .y= (v1.y>v2.y) ? v1.y : v2.y };
}
static inline vec2i v2i_clamp(vec2i v,vec2i min,vec2i max)
{
    return (vec2i){ .x= (v.x<min.x)?min.x:(v.x>max.x)?max.x:v.x,
                    .y= (v.y<min.y)?min.y:(v.y>max.y)?max.y:v.y };
}

//static inline vec2i v2i_min(vec2i v1,vec2i v2)
//{
//    return (vec2i){ .x= v1.x+(((v2.x-v1.x)>>31)&(v2.x-v1.x)),
//                    .y= v1.y+(((v2.y-v1.y)>>31)&(v2.y-v1.y))};
//}
//static inline vec2i v2i_max(vec2i v1,vec2i v2)
//{
//    return (vec2i){ .x= v1.x-(((v1.x-v2.x)>>31)&(v1.x-v2.x)),
//                    .y= v1.y-(((v1.y-v2.y)>>31)&(v1.y-v2.y))};
//}
//static inline vec2i v2i_clamp(vec2i v,vec2i min,vec2i max)
//{
//    return (vec2i){ .x= v.x-(((v.x-min.x)>>31)&(v.x-min.x))+(((max.x-v.x)>>31)&(max.x-v.x)),
//                    .y= v.y-(((v.y-min.y)>>31)&(v.y-min.y))+(((max.y-v.y)>>31)&(max.y-v.y))};
//}







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
    return sqrtf(v.x*v.x+v.y*v.y+v.z*v.z);
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
    return v3f_norm(v3f_add(v1,v2));
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
static inline vec3f v3f_from_spherical_direction(float zenith,float azimuth)
{
    return (vec3f){ .x=cosf(azimuth)*sinf(zenith),
                    .y=sinf(azimuth)*sinf(zenith),
                    .z=cosf(zenith)};
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
    return sqrtf(v.x*v.x+v.y*v.y);
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






static inline rectangle get_rectangle_overlap(rectangle r1,rectangle r2)
{
    r1.x1+=(r2.x1>r1.x1)*(r2.x1-r1.x1);
    r1.y1+=(r2.y1>r1.y1)*(r2.y1-r1.y1);

    r1.x2+=(r2.x2<r1.x2)*(r2.x2-r1.x2);
    r1.y2+=(r2.y2<r1.y2)*(r2.y2-r1.y2);

    return r1;
}

static inline bool rectangle_has_positive_area(rectangle r)
{
    return r.x2 > r.x1 && r.y2 > r.y1;
}

static inline rectangle rectangle_add_offset(rectangle r,int x,int y)
{
    return (rectangle){.x1=r.x1+x,.y1=r.y1+y,.x2=r.x2+x,.y2=r.y2+y};
}

static inline rectangle rectangle_subtract_offset(rectangle r,int x,int y)
{
    return (rectangle){.x1=r.x1-x,.y1=r.y1-y,.x2=r.x2-x,.y2=r.y2-y};
}

static inline bool rectangle_surrounds_point(rectangle r,vec2i p)
{
    return ((r.x1 <= p.x)&&(r.y1 <= p.y)&&(r.x2 > p.x)&&(r.y2> p.y));
}

static inline bool rectangle_surrounds_origin(rectangle r)
{
    return ((r.x1 <= 0)&&(r.y1 <= 0)&&(r.x2 > 0)&&(r.y2> 0));
}

static inline bool rectangles_overlap(rectangle r1,rectangle r2)
{
    //return !(r1.x1>=r2.x2 || r2.x1>=r1.x2 || r1.y1>=r2.y2 || r2.y1>=r1.y2);
    return r1.x1<r2.x2 && r2.x1<r1.x2 && r1.y1<r2.y2 && r2.y1<r1.y2;
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



static inline rotor3f r3f_from_mid_v3f(vec3f v1,vec3f v2)
{
    ///assumes both input vectors are normalised

    /// result is the geometric product of v1 and v2   (in that order; v1^v2)
    return(rotor3f)
    {
        .s=v1.x*v2.x+v1.y*v2.y+v1.z*v2.z,
        .xy=v1.x*v2.y-v1.y*v2.x,
        .yz=v1.y*v2.z-v1.z*v2.y,
        .zx=v1.z*v2.x-v1.x*v2.z
    };
}

static inline rotor3f r3f_from_v3f(vec3f v1,vec3f v2)
{
    ///assumes both input vectors are normalised

    /**
    ///figure out using normalised half angle

    \\\m is magnitude of half angle vector
    m=sqrt((v1.x+v2.x)*(v1.x+v2.x)+(v1.y+v2.y)*(v1.y+v2.y)+(v1.z+v2.z)*(v1.z+v2.z));
    m=sqrt(v1.x*v1.x+2*v1.x*v2.x+v2.x*v2.x + v1.y*v1.y+2*v1.y*v2.y+v2.y*v2.y + v1.z*v1.z+2*v1.z*v2.z+v2.z*v2.z);
    m=sqrt(v1.x*v1.x+v1.y*v1.y+v1.z*v1.z + v2.x*v2.x+v2.y*v2.y+v2.z*v2.z  + 2*(v1.x*v2.x+v1.y*v2.y+2*v1.z*v2.z)); dot(v1,v1)=dot(v2,v2)=1.0
    m=sqrt(2 + 2*(v1.x*v2.x + v1.y*v2.y + v1.z*v2.z));
    \\\d is dot product of 2 vectors
    m=sqrt(2+2*d);

    2+2*d   ///prove that this is magnitude of rotor+{1,0,0,0} squared
    scalar_component = d (by definition of rotor construction, scalar is cos(angle) = dot_product of 2 vectors to rotate between)
    geometric_component_mag ^ 2 = xy*xy+yz*yz+zx*zx = 1-d*d
    mag(rotor+{1,0,0,0})^2 = (scalar_component+1)^2 + geometric_component_mag ^ 2
    (1+d)*(1+d) + 1-d*d
    1+2*d+d*d + 1-d*d;
	2+2*d +d*d-d*d;
    2+2*d;
    \\\ergo
    mag(rotor+{1,0,0,0})=sqrt(2+2*d)

    now calculate v1 ^ (v1+v2)/m

    \\\S
    (v1.x*(v1.x+v2.x) + v1.y*(v1.y+v2.y) + v1.z*(v1.z+v2.z)) / m
    (v1.x*v1.x + v1.x*v2.x + v1.y*v1.y + v1.y*v2.y + v1.z*v1.z + v1.z*v2.z) / m
    (1 + v1.x*v2.x + v1.y*v2.y + v1.z*v2.z) / m
    (1+d)/m
    (1+d)/sqrt(2+2*d)
    (1+d)/mag(rotor+{1,0,0,0})

    \\\xy
    (v1.x*(v1.y+v2.y) - v1.y*(v1.x+v2.x)) / m
    (v1.x*v1.y + v1.x*v2.y - v1.y*v1.x - v1.y*v2.x) / m
    (v1.x*v2.y - v1.y*v2.x)/m    : v1 only terms cancel out
        : this is the same as the regular xy component of the exterior product of v1 and v2, just divided by m, i.e. divided by mag(rotor+{1,0,0,0})

    \\\yz same logic applies as did for xy
    \\\xz same logic applies as did for xy
    */
    float d=v1.x*v2.x + v1.y*v2.y + v1.z*v2.z;
    float im=1.0/sqrtf(2.0+2.0*d);///inverse_magnitude

    return(rotor3f)
    {
        .s=im+im*d,///(1+d)*im
        .xy=im*(v1.x*v2.y-v1.y*v2.x),
        .yz=im*(v1.y*v2.z-v1.z*v2.y),
        .zx=im*(v1.z*v2.x-v1.x*v2.z)
    };
}

static inline rotor3f r3f_from_v3f_and_angle(vec3f v,float a)
{
    ///assumes input vector is normalised
    float s=sinf(a*0.5);
    return(rotor3f){.s=cosf(a*0.5),.xy=s*v.z,.yz=s*v.x,.zx=s*v.y};
}

///in terms of applying rotations, LHS gets applied first
static inline rotor3f r3f_multiply(rotor3f r1,rotor3f r2)
{
    /// figured out via geometric product of r1 & r2
    return(rotor3f)
    {
        .s =r1.s*r2.s-r1.xy*r2.xy-r1.yz*r2.yz-r1.zx*r2.zx,
        .xy=r1.s*r2.xy+r1.xy*r2.s-r1.yz*r2.zx+r1.zx*r2.yz,
        .yz=r1.s*r2.yz+r1.xy*r2.zx+r1.yz*r2.s-r1.zx*r2.xy,
        .zx=r1.s*r2.zx-r1.xy*r2.yz+r1.yz*r2.xy+r1.zx*r2.s
    };
}

/// for these simplify multiplication as if r1 represents a rotation about the respective axes
/// treated as if a LHS multiplier (applied as rotation before extant rotation r)
static inline rotor3f r3f_rotate_around_x_axis(rotor3f r,float a)
{
    float s=sinf(a*0.5);
    float c=cosf(a*0.5);
    return (rotor3f){.s=c*r.s-s*r.yz, .xy=c*r.xy-s*r.zx, .yz=c*r.yz+s*r.s, .zx=c*r.zx+s*r.xy};
}
static inline rotor3f r3f_rotate_around_y_axis(rotor3f r,float a)
{
    float s=sinf(a*0.5);
    float c=cosf(a*0.5);
    return (rotor3f){.s=c*r.s-s*r.zx, .xy=c*r.xy+s*r.yz, .yz=c*r.yz-s*r.xy, .zx=c*r.zx+s*r.s};
}
static inline rotor3f r3f_rotate_around_z_axis(rotor3f r,float a)
{
    float s=sinf(a*0.5);
    float c=cosf(a*0.5);
    return (rotor3f){.s=c*r.s-s*r.xy, .xy=c*r.xy+s*r.s, .yz=c*r.yz+s*r.zx, .zx=c*r.zx-s*r.yz};
}

static inline matrix3f r3f_to_m3f(rotor3f r)
{
    /**
    // unoptimised included for educational purposes, is acquired by applying geometric product of rotor (r) over axis each axis(v) as -rvr
    // using geometric product & exterior product rules (r is a geometric product of 2 implict vectors, comprised of a bivector and a scalar)


    // here a,b,c represent the constants of their respective bivector components of the rotor; xy,yz,zx respectively
    // and f,g,h represent basis components of vector v; x,y,z respectively
    -r = s + ayx + bzy + cxz	//reverse order of bivector components of r to get "-r"
    r  = s + axy + byz + czx
    v  = fx + gy + hz

    -rv = s fx + s gy + s hz   +  ayx fx + ayx gy + ayx hz  +  bzy fx + bzy gy + bzy hz  +  cxz fx + cxz gy + cxz hz
	    = sf x + sg y + sh z   +  af yxx + ag yxy + ah yxz  +  bf zyx + bg zyy + bh zyz  +  cf xzx + cg xzy + ch xzz	// separate out sclar/constant components and put geometric compoents together
	    = sf x + sg y + sh z   +  af y   - ag x   - ah xyz  -  bf xyz + bg z   - bh y    -  cf z   - cg xyz + ch x  	// simplify and get unsimplifiable components in terms of xyz
	    = (sf - ag + ch) x   +  (sg + af - bh) y  +  (sh + bg - cf) z  -  (ah + bf + cg) xyz 							// reorganise to group all sclaras representing the same geometric components

	(-rv)r = -rvr
	-rvr = (sf s - ag s + ch s) x     +  (sg s + af s - bh s) y     +  (sh s + bg s - cf s) z     -  (ah s + bf s + cg s) xyz
	     + (sf a - ag a + ch a) x xy  +  (sg a + af a - bh a) y xy  +  (sh a + bg a - cf a) z xy  -  (ah a + bf a + cg a) xyz xy
	     + (sf b - ag b + ch b) x yz  +  (sg b + af b - bh b) y yz  +  (sh b + bg b - cf b) z yz  -  (ah b + bf b + cg b) xyz yz
	     + (sf c - ag c + ch c) x zx  +  (sg c + af c - bh c) y zx  +  (sh c + bg c - cf c) z zx  -  (ah c + bf c + cg c) xyz zx

		// simplify and get unsimplifiable components in terms of xyz
	     = (sf s - ag s + ch s) x    +  (sg s + af s - bh s) y    +  (sh s + bg s - cf s) z    -  (ah s + bf s + cg s) xyz
	     + (sf a - ag a + ch a) y    -  (sg a + af a - bh a) x    +  (sh a + bg a - cf a) xyz  +  (ah a + bf a + cg a) z	// xyzxy -> -xyxzy -> xyxyz -> -xyyxz -> -z
	     + (sf b - ag b + ch b) xyz  +  (sg b + af b - bh b) z    -  (sh b + bg b - cf b) y    +  (ah b + bf b + cg b) x	// xyzyz -> -xyzzy -> -x
	     - (sf c - ag c + ch c) z    +  (sg c + af c - bh c) xyz  +  (sh c + bg c - cf c) x    +  (ah c + bf c + cg c) y	// xyzzx -> -yxzzx -> -y

		// reorganise in terms of geometric components
		 = xyz (sfb - agb + chb  +  sgc + afc - bhc  +  sha + bga - cfa  -  ahs - bfs - cgs)
		 + x   (sfs - ags + chs  -  sga - afa + bha  +  shc + bgc - cfc  +  ahb + bfb + cgb)
		 + y   (sfa - aga + cha  +  sgs + afs - bhs  -  shb - bgb + cfb  +  ahc + bfc + cgc)
		 + z   (-sfc+ agc - chc  +  sgb + afb - bhb  +  shs + bgs - cfs  +  aha + bfa + cga)

		// cancel terms (remember scalar components can be reordered, due to oop scalar order always flipped above)
		 = xyz (0) // it cancels out completely
		 + x   (sfs - 2 ags + 2 chs - afa + 2 bha + 2 bgc - cfc + bfb)
		 + y   (2 sfa - aga + 2 cha + sgs - 2 bhs - bgb + 2 cfb + cgc)
		 + z   (-2 sfc+ 2 agc - chc + 2 sgb + 2 afb - bhb + shs + aha)

		// group in terms of basis components of vector (f,g,h)
		 = x   ( f (ss - aa + bb - cc)  +  g 2 (bc - as)  +  h 2 (cs + ba) )
		 + y   ( f 2 (sa + cb)  +  g (ss - aa - bb + cc)  +  h 2 (ca - bs) )
		 + z   ( f 2 (ab - sc)  +  g 2 (ac + sb)  +  h (ss + aa - bb - cc) )

        // substituting  ss + aa + bb + cc = 1  thus  1 - ss + aa + bb + cc = 0  to simplify
		 = x   ( f (1 - 2 aa - 2 cc)  +  g 2 (bc - as)  +  h 2 (cs + ba) )
		 + y   ( f 2 (sa + cb)  +  g (1 - 2 aa - 2 bb)  +  h 2 (ca - bs) )
		 + z   ( f 2 (ab - sc)  +  g 2 (ac + sb)  +  h (1 - 2 bb - 2 cc) )

	// now substite a,b,c = xy,yz,zx for rotor and f,g,h = x,y,z for vector and put in matrix form and note every rotor component is multiplied by another which are in turn multiplied by 2
    */

    r.s*=SQRT_2;
    r.xy*=SQRT_2;
    r.yz*=SQRT_2;
    r.zx*=SQRT_2;
    ///why does it seem to be transposed!!!

    return (matrix3f)
    {
        .x=
        {
            .x=1.0 - r.xy*r.xy - r.zx*r.zx,
            .y=r.zx*r.yz + r.xy*r.s,
            .z=r.xy*r.yz - r.zx*r.s
        },
        .y=
        {
            .x=r.yz*r.zx - r.s*r.xy,
            .y=1.0 - r.xy*r.xy - r.yz*r.yz,
            .z=r.xy*r.zx + r.yz*r.s
        },
        .z=
        {
            .x=r.yz*r.xy + r.s*r.zx,
            .y=r.zx*r.xy - r.yz*r.s,
            .z=1.0 - r.zx*r.zx - r.yz*r.yz
        }
    };
}

static inline vec3f r3f_rotate_v3f(rotor3f r,vec3f v)
{
    return(vec3f)
    {
        .x=2.0*(v.x*(0.5 - r.xy*r.xy - r.zx*r.zx) + v.y*(r.yz*r.zx - r.s*r.xy) + v.z*(r.yz*r.xy + r.s*r.zx)),
        .y=2.0*(v.x*(r.zx*r.yz + r.xy*r.s) + v.y*(0.5 - r.xy*r.xy - r.yz*r.yz) + v.z*(r.zx*r.xy - r.yz*r.s)),
        .z=2.0*(v.x*(r.xy*r.yz - r.zx*r.s) + v.y*(r.xy*r.zx + r.yz*r.s) + v.z*(0.5 - r.zx*r.zx - r.yz*r.yz))
    };
}

static inline vec3f r3f_derotate_v3f(rotor3f r,vec3f v)
{
    /// reverse the rotation that would be performed by r, can be figured out based on rotation operation of r on v (as in r3f_rotate_v3f)
    /// that rotation being represented/thought of as an orthonormal basis space transformation (a matrix)
    /// and the inverse of an orthonormal matrix is just its that matrix transposed!
    return(vec3f)
    {
        .x=2.0*(v.x*(0.5 - r.xy*r.xy - r.zx*r.zx) + v.y*(r.zx*r.yz + r.xy*r.s) + v.z*(r.xy*r.yz - r.zx*r.s)),
        .y=2.0*(v.x*(r.yz*r.zx - r.s*r.xy) + v.y*(0.5 - r.xy*r.xy - r.yz*r.yz) + v.z*(r.xy*r.zx + r.yz*r.s)),
        .z=2.0*(v.x*(r.yz*r.xy + r.s*r.zx) + v.y*(r.zx*r.xy - r.yz*r.s) + v.z*(0.5 - r.zx*r.zx - r.yz*r.yz))
    };
}

static inline vec3f r3f_get_x_axis(rotor3f r)
{
    return(vec3f){.x=1.0-2.0*(r.xy*r.xy + r.zx*r.zx),.y=2.0*(r.zx*r.yz + r.xy*r.s),.z=2.0*(r.xy*r.yz - r.zx*r.s)};
}

static inline vec3f r3f_get_y_axis(rotor3f r)
{
    return(vec3f){.x=2.0*(r.yz*r.zx - r.s*r.xy),.y=1.0-2.0*(r.xy*r.xy + r.yz*r.yz),.z=2.0*(r.xy*r.zx + r.yz*r.s)};
}

static inline vec3f r3f_get_z_axis(rotor3f r)
{
    return(vec3f){.x=2.0*(r.yz*r.xy + r.s*r.zx),.y=2.0*(r.zx*r.xy - r.yz*r.s),.z=1.0-2.0*(r.zx*r.zx + r.yz*r.yz)};
}


static inline rotor3f r3f_isolate_xy_rotation(rotor3f r)/// isolates the rotation about the z axis
{
    /**
    r = a + b xy + c yz + d zx

    // from r3f_get_z_axis
    z_axis = 2(cb+ad) x  +  2(db-ac) y  +  1-2(dd+cc) z


    // calculate rotation required to get this to  1,0,0  ( use r3f_from_v3f with v1 = 0,0,1 )
    dot_product = 1-2(dd+cc) //basically just the z component
    im=inv_magnitude
    im=1/sqrt(2+2(1-2(dd+cc)))
      =1/sqrt(4-4(dd+cc))
      =1/(2*sqrt(1-dd-cc))
      =0.5/sqrt(1-dd-cc) // both this and following will be useful, use whichever is most useful for given situation
      =0.5/sqrt(aa+bb) // aa+bb+cc+dd = 1   therefore   1-cc-dd = aa+bb

    z_rot=

    .s = (1+dot_product)*im
       = (2-2*dd-2*cc)*0.5/sqrt(1-dd-cc)
       = (1-dd-cc)/sqrt(1-dd-cc)
       = sqrt(1-dd-cc)
       = sqrt(aa+bb) // aa+bb+cc+dd = 1   therefore   1-cc-dd = aa+bb

    .xy = 0 // no v1.z, makes sense as we're avoiding this component

    .yz = -v2.y * im
        = -2(db-ac) * 0.5/sqrt(aa+bb)
        = (ac-db)/sqrt(aa+bb)

    .zx = v2.x * im
        = 2(cb+ad) * 0.5/sqrt(aa+bb)
        = (cb+ad)/sqrt(aa+bb)


    r3f_multiply(Ra,z_rot_inv); // z_rot_inv is just z_rot with xy,yz,zx multiplied by -1 (rotate opposite direction)

    .s  = a*sqrt(aa+bb)  +  c*(ac-db)/sqrt(aa+bb)  +  d*(cb+ad)/sqrt(aa+bb)
    .s*sqrt(aa+bb) = aaa+abb + acc-bcd + bcd+add
    .s  = (aaa+abb + acc+add)/sqrt(aa+bb)
        = a(aa+bb + cc+dd)/sqrt(aa+bb) //1=aa+bb+cc+dd
        = a/sqrt(aa+bb)

    .xy = b*sqrt(aa+bb)  +  c*(bc+ad)/sqrt(aa+bb)  +  d*(bd-ac)/sqrt(aa+bb)
    .xy*sqrt(aa+bb) = aab+bbb  +  bcc+acd + bdd-acd
    .xy = b(aa+bb+cc+dd)/sqrt(aa+bb) //1=aa+bb+cc+dd
    .xy = b/sqrt(aa+bb)

    .yz = a*(bd-ac)/sqrt(aa+bb) + b*(-bc-ad)/sqrt(aa+bb) + c*sqrt(aa+bb)
        = (abd-aac-bbc-abd)/sqrt(aa+bb) + c*sqrt(aa+bb)
        = (aac-bbc)/sqrt(aa+bb) + c*sqrt(aa+bb)
    .yz*sqrt(aa+bb) = aac-bbc+c*(aa+bb)
                    = aac-bbc+aac+bbc
                    = 0

    .zx = a*(-bc-ad)/sqrt(aa+bb) + b*(ac-bd)/sqrt(aa+bb) + d*sqrt(aa+bb)
        = (-abc-aad+abc-bbd)/sqrt(aa+bb) + d*sqrt(aa+bb)
        = (-aad-bbd)/sqrt(aa+bb) + d*sqrt(aa+bb)
    .zx*sqrt(aa+bb) = -aad-bbd + d*(aa+bb)
                    = -aad-bbd+aad+bbd
                    = 0
    */

    float im=1.0/sqrtf(r.s*r.s+r.xy*r.xy);///inverse magnitude
    return(rotor3f){.s=r.s*im,.xy=r.xy*im,.yz=0.0,.zx=0.0};
}

static inline rotor3f r3f_isolate_yz_rotation(rotor3f r)/// isolates the rotation about the x axis
{
    /// logic/maths the same as in r3f_isolate_xy_rotation, just for different bivector

    float im=1.0/sqrtf(r.s*r.s+r.yz*r.yz);///inverse magnitude
    return(rotor3f){.s=r.s*im,.xy=0.0,.yz=r.yz*im,.zx=0.0};
}

static inline rotor3f r3f_isolate_zx_rotation(rotor3f r)/// isolates the rotation about the y axis
{
    /// logic/maths the same as in r3f_isolate_xy_rotation, just for different bivector

    float im=1.0/sqrtf(r.s*r.s+r.zx*r.zx);///inverse magnitude
    return(rotor3f){.s=r.s*im,.xy=0.0,.yz=0.0,.zx=r.zx*im};
}

static inline rotor3f r3f_isolate_x_axis_rotation(rotor3f r)
{
    /**
    r = a + b xy + c yz + d zx

    // from r3f_get_y_axis
    x_axis = 1-2(bb+dd) x  +  2(cd+ab) y  +  2(bc-ad) z


    // calculate rotation required to get this to  1,0,0  ( use r3f_from_v3f with v1 = 1,0,0 )
    dot_product = 1-2(bb+dd) //basically just the x component
    im=inv_magnitude
    im=1/sqrt(2+2(1-2(bb+dd)))
      =1/sqrt(4-4(bb+dd))
      =1/(2*sqrt(1-bb-dd))
      =0.5/sqrt(1-bb-dd) // both this and following will be useful, use whichever is most useful for given situation
      =0.5/sqrt(aa+cc) // aa+bb+cc+dd = 1   therefore   1-cc-bb = aa+dd


    z_rot=

    .s = (1+dot_product)*im
       = (2-2*bb-2*dd)*0.5/sqrt(1-bb-dd)
       = (1-bb-dd)/sqrt(1-bb-dd)
       = sqrt(1-bb-dd)
       = sqrt(aa+cc) // aa+bb+cc+dd = 1   therefore   1-cc-bb = aa+dd

    .xy = v2.y * im
        = 2(cd+ab) * 0.5/sqrt(aa+cc)
        = (cd+ab)/sqrt(aa+cc)

    .yz = 0 // no v1.y, makes sense as we're avoiding this component

    .zx = -v2.z * im
        = 2(ad-bc) * 0.5/sqrt(aa+cc)
        = (ad-bc)/sqrt(aa+cc)
    */

    float m=sqrtf(r.s*r.s+r.yz*r.yz);
    float im=1.0/m;///inverse magnitude
    return(rotor3f){.s=m,.xy=(r.yz*r.zx+r.s*r.xy)*im,.yz=0.0,.zx=(r.s*r.zx-r.xy*r.yz)*im};
}

static inline rotor3f r3f_isolate_y_axis_rotation(rotor3f r)
{
    /**
    r = a + b xy + c yz + d zx

    // from r3f_get_y_axis
    y_axis = 2(cd-ab) x  +  1-2(bb+cc) y  +  2(bd+ac) z


    // calculate rotation required to get this to  0,1,0  ( use r3f_from_v3f with v1 = 0,1,0 )
    dot_product = 1-2(dd+cc) //basically just the y component
    im=inv_magnitude
    im=1/sqrt(2+2(1-2(bb+cc)))
      =1/sqrt(4-4(bb+cc))
      =1/(2*sqrt(1-bb-cc))
      =0.5/sqrt(1-bb-cc) // both this and following will be useful, use whichever is most useful for given situation
      =0.5/sqrt(aa+dd) // aa+bb+cc+dd = 1   therefore   1-cc-bb = aa+dd


    z_rot=

    .s = (1+dot_product)*im
       = (2-2*bb-2*cc)*0.5/sqrt(1-bb-cc)
       = (1-bb-cc)/sqrt(1-bb-cc)
       = sqrt(1-bb-cc)
       = sqrt(aa+dd) // aa+bb+cc+dd = 1   therefore   1-cc-bb = aa+dd

    .xy = -v2.x * im
        = -2(cd-ab) * 0.5/sqrt(aa+dd)
        = (ab-cd)/sqrt(aa+dd)

    .yz = v2.z * im
        = 2(bd+ac) * 0.5/sqrt(aa+dd)
        = (bd+ac)/sqrt(aa+dd)

    .zx = 0 // no v1.y, makes sense as we're avoiding this component
    */

    float m=sqrtf(r.s*r.s+r.zx*r.zx);
    float im=1.0/m;///inverse magnitude
    return(rotor3f){.s=m,.xy=(r.s*r.xy-r.yz*r.zx)*im,.yz=(r.xy*r.zx+r.s*r.yz)*im,.zx=0.0};
}

static inline rotor3f r3f_isolate_z_axis_rotation(rotor3f r)
{
    /**
    r = a + b xy + c yz + d zx

    // from r3f_get_z_axis
    z_axis = 2(cb+ad) x  +  2(db-ac) y  +  1-2(dd+cc) z


    // calculate rotation required to get this to  0,0,1  ( use r3f_from_v3f with v1 = 0,0,1 )
    dot_product = 1-2(dd+cc) //basically just the z component
    im=inv_magnitude
    im=1/sqrt(2+2(1-2(dd+cc)))
      =1/sqrt(4-4(dd+cc))
      =1/(2*sqrt(1-dd-cc))
      =0.5/sqrt(1-dd-cc) // both this and following will be useful, use whichever is most useful for given situation
      =0.5/sqrt(aa+bb) // aa+bb+cc+dd = 1   therefore   1-cc-dd = aa+bb


    z_rot=

    .s = (1+dot_product)*im
       = (2-2*dd-2*cc)*0.5/sqrt(1-dd-cc)
       = (1-dd-cc)/sqrt(1-dd-cc)
       = sqrt(1-dd-cc)
       = sqrt(aa+bb) // aa+bb+cc+dd = 1   therefore   1-cc-dd = aa+bb

    .xy = 0 // no v1.z, makes sense as we're avoiding this component

    .yz = -v2.y * im
        = -2(db-ac) * 0.5/sqrt(aa+bb)
        = (ac-db)/sqrt(aa+bb)

    .zx = v2.x * im
        = 2(cb+ad) * 0.5/sqrt(aa+bb)
        = (cb+ad)/sqrt(aa+bb)
    */

    float m=sqrtf(r.s*r.s+r.xy*r.xy);
    float im=1.0/m;///inverse magnitude
    return(rotor3f){.s=m,.xy=0.0,.yz=(r.s*r.yz-r.zx*r.xy)*im,.zx=(r.xy*r.yz +r.s*r.zx)*im};
}

///useful for ensuring object rotation doesn't start introducing skew as error accumulates over time
static inline rotor3f r3f_normalize(rotor3f r)
{
    float im=1.0/sqrtf(r.s*r.s+r.xy*r.xy+r.yz*r.yz+r.zx*r.zx);///inverse magnitude
    return(rotor3f){.s=r.s*im,.xy=r.xy*im,.yz=r.yz*im,.zx=r.zx*im};
}

static inline rotor3f r3f_lerp(rotor3f r1,rotor3f r2,float t)
{
    float u;
    u=1.0-t;

    r1.s =r1.s *u+r2.s *t;
    r1.xy=r1.xy*u+r2.xy*t;
    r1.yz=r1.yz*u+r2.yz*t;
    r1.zx=r1.zx*u+r2.zx*t;

    u=1.0/sqrtf(r1.s*r1.s+r1.xy*r1.xy+r1.yz*r1.yz+r1.zx*r1.zx);
    r1.s*=u,r1.xy*=u,r1.yz*=u,r1.zx*=u;

    return r1;
}

static inline rotor3f r3f_bezier_interp(rotor3f r0,rotor3f r1,rotor3f r2,rotor3f r3,float t)
{
    float u,uut,utt;

    u=1.0-t;
    uut=u*u*t*3.0;
    utt=u*t*t*3.0;
    t*=t*t;
    u*=u*u;

    r1.s =r1.s *u+(r1.s +(r2.s -r0.s )*0.1666)*uut+(r2.s +(r1.s -r3.s )*0.1666)*utt+r2.s *t;
    r1.xy=r1.xy*u+(r1.xy+(r2.xy-r0.xy)*0.1666)*uut+(r2.xy+(r1.xy-r3.xy)*0.1666)*utt+r2.xy*t;
    r1.yz=r1.yz*u+(r1.yz+(r2.yz-r0.yz)*0.1666)*uut+(r2.yz+(r1.yz-r3.yz)*0.1666)*utt+r2.yz*t;
    r1.zx=r1.zx*u+(r1.zx+(r2.zx-r0.zx)*0.1666)*uut+(r2.zx+(r1.zx-r3.zx)*0.1666)*utt+r2.zx*t;

    u=1.0/sqrtf(r1.s*r1.s+r1.xy*r1.xy+r1.yz*r1.yz+r1.zx*r1.zx);
    r1.s*=u,r1.xy*=u,r1.yz*=u,r1.zx*=u;

    return r1;
}

static inline rotor3f r3f_spherical_interp(rotor3f r1,rotor3f r2,float t)
{
    ///dont yet have derivation for this
    ///may want to extend to r3f_bezier_spherical_interp

    float u,m,d;

    m=acosf(r1.s*r2.s+r1.xy*r2.xy+r1.yz*r2.yz+r1.zx*r2.zx);
    d=1.0/sin(m);

    u=sin((1.0-t)*m)*d;
    t=sin(t*m)*d;

    r1.s =r1.s *u+r2.s *t;
    r1.xy=r1.xy*u+r2.xy*t;
    r1.yz=r1.yz*u+r2.yz*t;
    r1.zx=r1.zx*u+r2.zx*t;

    return r1;
}






#endif

