/**
Copyright 2020,2021,2022,2023 Carl van Mastrigt

This file is part of solipsix.

solipsix is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

solipsix is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with solipsix.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef solipsix_H
#include "solipsix.h"
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
#define SQRT_THIRD  0.5f773502691896257645091487805019574556476017512701268760186023264
#endif



/// fmaxf should be used to replace the ternaries here

#warning FFS _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(v))) -> _mm_shuffle_ps(v) (just use same vector!)
#warning use _MM_SHUFFLE to set bitfields!

/// no intrinsics for vec2f available (on sse at least)
typedef struct vec2f
{
    float x;
    float y;
}
vec2f;

static inline vec2f v2f_ini(float x,float y)
{
    return (vec2f){.x=x,.y=y};
}
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
    vec2f v;
    sincosf(angle,&v.y,&v.x);
    return v;
}
static inline float v2f_angle(vec2f v)
{
    return atan2f(v.y,v.x);
}
static inline vec2f v2f_rotate(vec2f v,float angle)
{
    float s,c;
    sincosf(angle,&s,&c);
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



typedef struct mat22f
{
    vec2f x;
    vec2f y;
}
mat22f;

static inline vec2f m22f_v2f_mul(mat22f m,vec2f v)
{
    assert(false);///needs to be converted to vector/intrinsic ops, though really would to prefer not using m2x2
    return (vec2f){.x=v.x*m.x.x+v.y*m.y.x,.y=v.x*m.x.y+v.y*m.y.y};
}
static inline vec2f v2f_m32f_mul(mat22f m,vec2f v)
{
    assert(false);///needs to be converted to vector/intrinsic ops, though really would to prefer not using m2x2
    return (vec2f){.x=v.x*m.x.x+v.y*m.x.y,.y=v.x*m.y.x+v.y*m.y.y};
}
static inline mat22f m2f_rotation_matrix(float angle)
{
    float s,c;
    sincosf(angle,&s,&c);
    assert(false);///needs to be converted to vector/intrinsic ops, though really would to prefer not using m2x2
    mat22f m;
    m.x.x=m.y.y=c;
    m.x.y= -(m.y.x=s);
    return m;
}



/// insufficient intrinsics for vec2i to be meaningful on sse
typedef struct vec2i
{
    int32_t x;
    int32_t y;
}
vec2i;

///vec2i
static inline vec2i v2i_ini(int32_t x,int32_t y)
{
    return (vec2i){.x=x,.y=y};
}
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
//    return (vec2i){ .x= v1.x+(((v2.x-v1.x)>>31)&(v2.x-v1.x)),
//                    .y= v1.y+(((v2.y-v1.y)>>31)&(v2.y-v1.y))};
    return (vec2i){.x=(v1.x<v2.x)?v1.x:v2.x,.y=(v1.y<v2.y)?v1.y:v2.y};
}
static inline vec2i v2i_max(vec2i v1,vec2i v2)
{
//    return (vec2i){ .x= v1.x-(((v1.x-v2.x)>>31)&(v1.x-v2.x)),
//                    .y= v1.y-(((v1.y-v2.y)>>31)&(v1.y-v2.y))};
    return (vec2i){.x=(v1.x>v2.x)?v1.x:v2.x,.y=(v1.y>v2.y)?v1.y:v2.y};
}
static inline vec2i v2i_clamp(vec2i v,vec2i min,vec2i max)
{
//    return (vec2i){ .x= v.x-(((v.x-min.x)>>31)&(v.x-min.x))+(((max.x-v.x)>>31)&(max.x-v.x)),
//                    .y= v.y-(((v.y-min.y)>>31)&(v.y-min.y))+(((max.y-v.y)>>31)&(max.y-v.y))};
    return (vec2i){.x=(v.x<min.x)?min.x:(v.x>max.x)?max.x:v.x,.y=(v.y<min.y)?min.y:(v.y>max.y)?max.y:v.y};
}






typedef struct rectangle
{
    int16_t x1;
    int16_t y1;
    int16_t x2;
    int16_t y2;
}
rectangle;

///rectangle
static inline rectangle rectangle_ini(int16_t x1,int16_t y1,int16_t x2,int16_t y2)
{
    return (rectangle){.x1=x1,.y1=y1,.x2=x2,.y2=y2};
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
static inline rectangle rectangle_add_offset(rectangle r,int16_t x,int16_t y)
{
    return (rectangle){.x1=r.x1+x,.y1=r.y1+y,.x2=r.x2+x,.y2=r.y2+y};
}
static inline rectangle rectangle_subtract_offset(rectangle r,int16_t x,int16_t y)
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
    return r1.x1<r2.x2 && r2.x1<r1.x2 && r1.y1<r2.y2 && r2.y1<r1.y2;
}







///none of these should be defined in retail except CVM_INTRINSIC_MODE_NONE which is useful if issues arise
///can consider using CVM_INTRINSIC_MODE_NONE in debug (or "fast debug") mode as moving between register types is a nontrivial expense and does slow doen execution
//#define CVM_INTRINSIC_MODE_NONE
#define CVM_INTRINSIC_MODE_SSE /*this can trick IDE, when it doesn't pick up __SSE__ :p*/



#if (defined __SSE__ || defined CVM_INTRINSIC_MODE_SSE) && !defined CVM_INTRINSIC_MODE_NONE
/**====================== SSE INTRINSIC IMPLEMENTATION ======================*/
typedef union vec3i
{
    __m128i v;
    struct
    {
        int32_t x;
        int32_t y;
        int32_t z;
        int32_t dummy_w;
    };
}
vec3i;

///vec3i
static inline vec3i v3i_ini(int32_t x,int32_t y,int32_t z)
{
    return (vec3i){.v=_mm_set_epi32(0,z,y,x)};
}
static inline vec3i v3i_add(vec3i v1,vec3i v2)
{
    return (vec3i){.v=_mm_add_epi32(v1.v,v2.v)};
}
static inline vec3i v3i_sub(vec3i v1,vec3i v2)
{
    return (vec3i){.v=_mm_sub_epi32(v1.v,v2.v)};
}
static inline vec3i v3i_mul(vec3i v,int32_t c)
{
    return (vec3i){.v=_mm_mullo_epi32(v.v,_mm_set1_epi32(c))};
}
static inline vec3i v3i_div(vec3i v,int32_t c)
{
    ///only 10-15% faster than per component division...
    ///will produce undefined behaviour when (d==1 or d==-1) and n==INT_MIN , these cases COULD be handled by returning n or n*d in those cases, but i don't see such support as worth the performance hit
    /// ESPECIALLY when INT_MIN/-1 is an invalid op anyway! (so really only real op is unsupported)
    /// ESPECIALLY ESPECIALLY when that "undefined behaviour" actually works on all platforms...
    int32_t sh;
    int64_t ad;
    __m128i mm,h,l,r,sm,de;
    sm=_mm_set1_epi32(c>>31);///propagate sign mask of divisor
    ad=labs((int64_t)c);///absolute value of divisor
    sh=cvm_po2_64_lt(ad);
    int64_t m=((((int64_t)1)<<(sh|0x20))/ad)-(((int64_t)1)<<32)+1;///subtract 2^32 (then add v later before right shifting 32) to ensure multiplier stays withing s32 range

    mm=_mm_set1_epi32(m);///propagate multiplier to vector
    de=_mm_add_epi32(_mm_srli_epi32(v.v,31),sm);///sm is (fortuitously) just negative 1 times sign bit :D,
    ///this is values sign bits (to round division correctly) minus 1 if the divisor is negative to account for 2's compliment in xor based "negation" at end

    h=_mm_mul_epi32(_mm_srli_epi64(v.v,32),mm);///do high bits and leave in high bits
    l=_mm_srli_epi64(_mm_mul_epi32(v.v,mm),32);///do low bits and put in low bits

    r=_mm_blend_epi16(h,l,0x33);///combine low and high results
    r=_mm_add_epi32(r,v.v);///add v again as mentioned above to account
    r=_mm_srai_epi32(r,sh);///perform shift necessary to effectively "perform" division
    r=_mm_add_epi32(r, de);///account for sign BS
    return (vec3i){.v=_mm_xor_si128(r,sm)};///flip based on sign, other part of negation along with de
}
static inline vec3i v3i_mod(vec3i v,int32_t c)///because of reliance on v3i_div, mod suffers same limitation
{
    return v3i_sub(v,v3i_mul(v3i_div(v,c),c));
}
static inline vec3i v3i_rsh(vec3i v,int32_t c)
{
    return (vec3i){.v=_mm_srai_epi32(v.v,c)};
}
static inline vec3i v3i_lsh(vec3i v,int32_t c)
{
    return (vec3i){.v=_mm_slli_epi32(v.v,c)};
}
static inline vec3i v3i_min(vec3i v1,vec3i v2)
{
    return (vec3i){.v=_mm_min_epi32(v1.v,v2.v)};
}
static inline vec3i v3i_max(vec3i v1,vec3i v2)
{
    return (vec3i){.v=_mm_max_epi32(v1.v,v2.v)};
}
static inline vec3i v3i_clamp(vec3i v,vec3i min,vec3i max)
{
    return (vec3i){.v=_mm_min_epi32(_mm_max_epi32(v.v,min.v),max.v)};
}
static inline vec3i v3i_clamp_range(vec3i v,int32_t min,int32_t max)
{
    return (vec3i){.v=_mm_min_epi32(_mm_max_epi32(v.v,_mm_set1_epi32(min)),_mm_set1_epi32(max))};
}



typedef union vec3f
{
    __m128 v;
    struct
    {
        float x;
        float y;
        float z;
        float dummy_w;
    };
}
vec3f;

///vec3f
static inline vec3f v3f_ini(float x,float y,float z)
{
    return (vec3f){.v=_mm_set_ps(0.0f,z,y,x)};
}
static inline vec3f v3f_add(vec3f v1,vec3f v2)
{
    return (vec3f){.v=_mm_add_ps(v1.v,v2.v)};
}
static inline vec3f v3f_sub(vec3f v1,vec3f v2)
{
    return (vec3f){.v=_mm_sub_ps(v1.v,v2.v)};
}
static inline vec3f v3f_mul(vec3f v,float c)
{
    return (vec3f){.v=_mm_mul_ps(v.v,_mm_set1_ps(c))};
}
static inline vec3f v3f_div(vec3f v,float c)
{
    return (vec3f){.v=_mm_div_ps(v.v,_mm_set1_ps(c))};
}
static inline float v3f_dot(vec3f v1,vec3f v2)
{
    return _mm_cvtss_f32(_mm_dp_ps(v1.v,v2.v,0x71));
}
static inline float v3f_mag_sqr(vec3f v)
{
    return v3f_dot(v,v);
}
static inline float v3f_mag(vec3f v)
{
    return sqrtf(v3f_dot(v,v));
}
static inline vec3f v3f_nrm(vec3f v)
{
    return (vec3f){.v=_mm_div_ps(v.v,_mm_sqrt_ps(_mm_dp_ps(v.v,v.v,0x77)))};
}
static inline vec3f v3f_nrm_fst(vec3f v)///fast (but more inaccurate) variant (only true if using sse or similar intrinsic set)
{
    return (vec3f){.v=_mm_mul_ps(v.v,_mm_rsqrt_ps(_mm_dp_ps(v.v,v.v,0x77)))};
}
static inline float v3f_dst_sqr(vec3f v1,vec3f v2)
{
    return v3f_mag_sqr(v3f_sub(v1,v2));
}
static inline float v3f_dst(vec3f v1,vec3f v2)
{
    return v3f_mag(v3f_sub(v1,v2));
}
static inline vec3f v3f_max(vec3f v1,vec3f v2)
{
    return (vec3f){.v=_mm_max_ps(v1.v,v2.v)};
}
static inline vec3f v3f_min(vec3f v1,vec3f v2)
{
    return (vec3f){.v=_mm_min_ps(v1.v,v2.v)};
}
static inline vec3f v3f_clamp_range(vec3f v,float min,float max)
{
    return (vec3f){.v=_mm_max_ps(_mm_min_ps(v.v,_mm_set1_ps(max)),_mm_set1_ps(min))};
}
static inline vec3f v3f_cross(vec3f v1,vec3f v2)
{
    return (vec3f){.v=_mm_sub_ps(
        _mm_mul_ps(_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(v1.v),0xC9))/** yzxw*/,_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(v2.v),0xD2))/** zxyw*/),
        _mm_mul_ps(_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(v1.v),0xD2))/** zxyw*/,_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(v2.v),0xC9))/** yzxw*/))};
}
static inline vec3f v3f_rotate(vec3f v,vec3f k,float cos_theta,float sin_theta)///Rodrigues rotation
{
    return v3f_add(v3f_add(v3f_mul(v,cos_theta),v3f_mul(v3f_cross(k,v),sin_theta)),
        (vec3f){.v=_mm_mul_ps(_mm_mul_ps(k.v,_mm_dp_ps(k.v,v.v,0x77)),_mm_set1_ps(1.0f-cos_theta))});
}
static inline vec3f v3f_from_spherical_direction(float zenith,float azimuth)
{
    float sz,cz,sa,ca;
    sincosf(zenith,&sz,&cz);
    sincosf(azimuth,&sa,&ca);
    return (vec3f){.v=_mm_mul_ps(_mm_set_ps(0.0f,cz,sa,ca),_mm_set_ps(0.0f,1.0,sz,sz))};///be cognizant of changes here if switching back to w first
}
static inline vec3f v3f_from_spherical(float r,float zenith,float azimuth)
{
    return v3f_mul(v3f_from_spherical_direction(zenith,azimuth),r);
}



typedef union vec4f
{
    __m128 v;
    struct
    {
        float x;
        float y;
        float z;
        float w;
    };
}
vec4f;

///vec4f
static inline vec4f v4f_ini(float x,float y,float z,float w)
{
    return (vec4f){.v=_mm_set_ps(w,z,y,x)};
}
static inline vec4f v4f_add(vec4f v1,vec4f v2)
{
    return (vec4f){.v=_mm_add_ps(v1.v,v2.v)};
}
static inline vec4f v4f_sub(vec4f v1,vec4f v2)
{
    return (vec4f){.v=_mm_sub_ps(v1.v,v2.v)};
}
static inline vec4f v4f_mul(vec4f v,float c)
{
    return (vec4f){.v=_mm_mul_ps(v.v,_mm_set1_ps(c))};
}
static inline vec4f v4f_div(vec4f v,float c)
{
    return (vec4f){.v=_mm_div_ps(v.v,_mm_set1_ps(c))};
}
static inline float v4f_dot(vec4f v1,vec4f v2)
{
    return _mm_cvtss_f32(_mm_dp_ps(v1.v,v2.v,0xF1));
}
static inline float v4f_mag_sqr(vec4f v)
{
    return v4f_dot(v,v);
}
static inline float v4f_mag(vec4f v)
{
    return sqrtf(v4f_dot(v,v));
}
static inline vec4f v4f_nrm(vec4f v)
{
    return (vec4f){.v=_mm_div_ps(v.v,_mm_sqrt_ps(_mm_dp_ps(v.v,v.v,0xFF)))};
}
static inline vec4f v4f_nrm_fst(vec4f v)///fast (but more inaccurate) variant (only true if using sse or similar intrinsic set)
{
    return (vec4f){.v=_mm_mul_ps(v.v,_mm_rsqrt_ps(_mm_dp_ps(v.v,v.v,0xFF)))};
}
static inline float v4f_dst_sqr(vec4f v1,vec4f v2)
{
    return v4f_mag_sqr(v4f_sub(v1,v2));
}
static inline float v4f_dst(vec4f v1,vec4f v2)
{
    return v4f_mag(v4f_sub(v1,v2));
}
static inline vec4f v4f_max(vec4f v1,vec4f v2)
{
    return (vec4f){.v=_mm_max_ps(v1.v,v2.v)};
}
static inline vec4f v4f_min(vec4f v1,vec4f v2)
{
    return (vec4f){.v=_mm_min_ps(v1.v,v2.v)};
}
static inline vec4f v4f_clamp_range(vec4f v,float min,float max)
{
    return (vec4f){.v=_mm_max_ps(_mm_min_ps(v.v,_mm_set1_ps(max)),_mm_set1_ps(min))};
}
static inline vec4f vec4f_blend(vec4f b,vec4f f)///treats w as alpha
{
    ///implements default blending equation with alpha: glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    __m128 am=_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(f.v),0xFF));///copy w into all components of alpha mask
    __m128 r=_mm_sub_ps(b.v,_mm_mul_ps(b.v,am));///do background component of blending
    am=_mm_blend_ps(am,_mm_set1_ps(1.0f),0x08);///change alpha mask for foreground blend
    return (vec4f){.v= _mm_add_ps(r,_mm_mul_ps(f.v,am))};///add foreground component
}



typedef union plane
{
    __m128 p;
    struct
    {
        float x;
        float y;
        float z;
        float dist;
    };
}
plane;

///plane
static inline plane plane_from_normal_and_point(vec3f n,vec3f p)
{
    __m128 dp=_mm_dp_ps(n.v,p.v,0x78);
    return (plane){.p=_mm_blend_ps(n.v,dp,0x08)};
}
static inline float plane_point_dist(plane p,vec3f point)
{
    __m128 cmp=_mm_blend_ps(point.v,_mm_set1_ps(-1.0f),0x08);
    return _mm_cvtss_f32(_mm_dp_ps(p.p,cmp,0xF1));
}




///for matrices vector x,y,z,w represents the vector that those respective euclidean basis vector map to
typedef struct mat44f
{
    vec4f x;
    vec4f y;
    vec4f z;
    vec4f w;
}
mat44f;

///mat44f
static inline mat44f m44f_transpose(mat44f m)
{
    __m128 xx_yx_xy_yy=_mm_castsi128_ps(_mm_unpacklo_epi32(_mm_castps_si128(m.x.v),_mm_castps_si128(m.y.v)));
    __m128 zx_wx_zy_wy=_mm_castsi128_ps(_mm_unpacklo_epi32(_mm_castps_si128(m.z.v),_mm_castps_si128(m.w.v)));

    __m128 xz_yz_xw_yw=_mm_castsi128_ps(_mm_unpackhi_epi32(_mm_castps_si128(m.x.v),_mm_castps_si128(m.y.v)));
    __m128 zz_wz_zw_ww=_mm_castsi128_ps(_mm_unpackhi_epi32(_mm_castps_si128(m.z.v),_mm_castps_si128(m.w.v)));

    m.x.v=_mm_movelh_ps(xx_yx_xy_yy,zx_wx_zy_wy);
    m.y.v=_mm_movehl_ps(zx_wx_zy_wy,xx_yx_xy_yy);
    m.z.v=_mm_movelh_ps(xz_yz_xw_yw,zz_wz_zw_ww);
    m.w.v=_mm_movehl_ps(zz_wz_zw_ww,xz_yz_xw_yw);

    return m;
}
static inline vec4f m44f_v4f_mul(mat44f m,vec4f v)
{
    return (vec4f){.v=_mm_add_ps(
        _mm_add_ps(
            _mm_mul_ps(m.x.v,_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(v.v),0x00))),
            _mm_mul_ps(m.y.v,_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(v.v),0x55)))),
        _mm_add_ps(
            _mm_mul_ps(m.z.v,_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(v.v),0xAA))),
            _mm_mul_ps(m.w.v,_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(v.v),0xFF)))))};
}
static inline vec3f m44f_v4f_mul_p(mat44f m,vec4f v)
{
    __m128 s=_mm_add_ps(
        _mm_add_ps(
            _mm_mul_ps(m.x.v,_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(v.v),0x00))),
            _mm_mul_ps(m.y.v,_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(v.v),0x55)))),
        _mm_add_ps(
            _mm_mul_ps(m.z.v,_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(v.v),0xAA))),
            _mm_mul_ps(m.w.v,_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(v.v),0xFF)))));

    return (vec3f){.v=_mm_div_ps(s,_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(s),0xFF)))};
}



typedef struct mat33f
{
    vec3f x;
    vec3f y;
    vec3f z;
}
mat33f;

static inline mat33f m33f_transpose(mat33f m)
{
    __m128 dummy=m.x.v;///doesnt matter what it gets set as

    __m128 xx_yx_xy_yy=_mm_castsi128_ps(_mm_unpacklo_epi32(_mm_castps_si128(m.x.v),_mm_castps_si128(m.y.v)));
    __m128 zx_wx_zy_wy=_mm_castsi128_ps(_mm_unpacklo_epi32(_mm_castps_si128(m.z.v),_mm_castps_si128(dummy)));

    __m128 xz_yz_xw_yw=_mm_castsi128_ps(_mm_unpackhi_epi32(_mm_castps_si128(m.x.v),_mm_castps_si128(m.y.v)));
    __m128 zz_wz_zw_ww=_mm_castsi128_ps(_mm_unpackhi_epi32(_mm_castps_si128(m.z.v),_mm_castps_si128(dummy)));

    m.x.v=_mm_movelh_ps(xx_yx_xy_yy,zx_wx_zy_wy);
    m.y.v=_mm_movehl_ps(zx_wx_zy_wy,xx_yx_xy_yy);
    m.z.v=_mm_movelh_ps(xz_yz_xw_yw,zz_wz_zw_ww);

    return m;
}
static inline vec3f m33f_v3f_mul(mat33f m,vec3f v)
{
    return (vec3f){.v=_mm_add_ps(_mm_add_ps(
            _mm_mul_ps(m.x.v,_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(v.v),0x00))),
            _mm_mul_ps(m.y.v,_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(v.v),0x55)))),
            _mm_mul_ps(m.z.v,_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(v.v),0xAA))))};
}



typedef union rotor3f
{
    __m128 r;
    struct
    {
        float s;
        float xy;
        float yz;
        float zx;
    };
}
rotor3f;

///rotor3f - see reference implementation for explaination of relevant maths
static inline rotor3f r3f_ini(float s,float xy,float yz,float zx)
{
    return (rotor3f){.r=_mm_set_ps(zx,yz,xy,s)};
}
static inline float r3f_dot(rotor3f r1,rotor3f r2)
{
    return _mm_cvtss_f32(_mm_dp_ps(r1.r,r2.r,0xF1));
}
static inline rotor3f r3f_nrm(rotor3f r)///intentionally very accurate as this is used for renormalization
{
    return (rotor3f){.r=_mm_div_ps(r.r,_mm_sqrt_ps(_mm_dp_ps(r.r,r.r,0xFF)))};
}
static inline rotor3f r3f_from_mid_v3f(vec3f v1,vec3f v2)
{
    ///assumes both input vectors are normalised

    return (rotor3f){.r=_mm_blend_ps(
        _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(_mm_sub_ps(///calculate bivector product in xyz and move up 1 element
            _mm_mul_ps(v1.v,_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(v2.v),0xC9))/** yzxw*/),
            _mm_mul_ps(v2.v,_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(v1.v),0xC9))/** yzxw*/))),0x93/**wxyz*/)),
        _mm_dp_ps(v1.v,v2.v,0x71),0x01)};
}
static inline rotor3f r3f_from_v3f(vec3f v1,vec3f v2)
{
    ///assumes both input vectors are normalised

    __m128 d1=_mm_add_ps(_mm_dp_ps(v1.v,v2.v,0x7F),_mm_set1_ps(1.0f));
    __m128 im=_mm_div_ps(_mm_set1_ps(SQRT_HALF),_mm_sqrt_ps(d1));///1.0f/sqrt(2.0f+2.0f*d) == sqrt_half/sqrt(1+d)

    return (rotor3f){.r=_mm_mul_ps(_mm_blend_ps(
        _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(_mm_sub_ps(///calculate bivector product in xyz and move up 1 element
            _mm_mul_ps(v1.v,_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(v2.v),0xC9))/** yzxw*/),
            _mm_mul_ps(v2.v,_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(v1.v),0xC9))/** yzxw*/))),0x93/**wxyz*/)),
        d1,0x01),im)};
}
static inline rotor3f r3f_from_angle(vec3f v,float a)
{
    float s,c;
    sincosf(a*0.5f,&s,&c);
    return (rotor3f){.r=_mm_blend_ps(
        _mm_mul_ps(_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(v.v),0x4B/**wzxy*/)),_mm_set1_ps(s)),
        _mm_set1_ps(c),0x01)};
}
///in terms of applying rotations, LHS gets applied first
static inline rotor3f r3f_mul(rotor3f r1,rotor3f r2)
{
    return (rotor3f){.r=_mm_xor_ps(_mm_set_ss(-0.0f),_mm_add_ps(
    _mm_add_ps(
        _mm_mul_ps(_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r1.r),0x2D)),_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r2.r),0xC9))),
        _mm_mul_ps(_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r1.r),0xD2)),_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r2.r),0x36)))),
    _mm_sub_ps(
        _mm_mul_ps(_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r1.r),0x87)),_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r2.r),0x63))),
        _mm_mul_ps(_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r1.r),0x78)),_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r2.r),0x9C))))))};
/**
     +
     1 = xy,zx,yz,s 1320 2D    2 = xy,yz,s,zx 1203 C9
     1 = yz,s,xy,zx 2013 D2    2 = yz,xy,zx,s 2130 36
     1 = zx,xy,s,yz 3102 87    2 = zx,s,yz,xy 3021 63

     -
     1 = s,yz,zx,xy 0231 78    2 = s,zx,xy,yz 0312 9C

     note: s components were inverted to make +/- ratio match, ergo the s sign flip at the end
*/
}
static inline rotor3f r3f_inv(rotor3f r)
{
    return (rotor3f){.r=_mm_xor_ps(_mm_set_ps(-0.0f,-0.0f,-0.0f,0.0f),r.r)};
}
static inline mat33f r3f_to_m33f(rotor3f r)
{
    __m128 r2=_mm_mul_ps(r.r,_mm_set1_ps(SQRT_2));///replacing this with "fast exponent" multiplication of each column by 2 provides only very marginal benefit, so deemed not worth it
    ///only putting s in last component because something has to go there
    __m128 xy_yz_zx=_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r2),0x39));/// xy,yz,zx,s
    __m128 zx_xy_yz=_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r2),0x27));/// zx,xy,yz,s
    __m128 yz_zx_xy=_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r2),0x1E));/// yz,zx,xy,s
    __m128 s3=_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r2),0x00));/// s,s,s,s

    __m128 s=_mm_sub_ps(_mm_sub_ps(_mm_set1_ps(1.0f),_mm_mul_ps(xy_yz_zx,xy_yz_zx)),_mm_mul_ps(zx_xy_yz,zx_xy_yz));

    __m128 p=_mm_add_ps(_mm_mul_ps(yz_zx_xy,xy_yz_zx),_mm_mul_ps(zx_xy_yz,s3));

    __m128 n=_mm_sub_ps(_mm_mul_ps(yz_zx_xy,zx_xy_yz),_mm_mul_ps(xy_yz_zx,s3));

    return (mat33f)
    {
        .x={.v=_mm_blend_ps(_mm_blend_ps(s,p,0x02),n,0x04)},
        .y={.v=_mm_blend_ps(_mm_blend_ps(s,p,0x04),n,0x01)},
        .z={.v=_mm_blend_ps(_mm_blend_ps(s,p,0x01),n,0x02)}
    };
}
static inline vec3f r3f_rotate_v3f(rotor3f r,vec3f v)
{
    //__m128 v2=_mm_castsi128_ps(_mm_add_epi32(_mm_castps_si128(v.v),_mm_set1_epi32(0x00800000)));
    /// above would alter zero entries of v to become non-zero, so considered unuseable
    __m128 v2=_mm_mul_ps(v.v,_mm_set1_ps(2.0));

    __m128 xy_yz_zx=_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r.r),0x39));/// xy,yz,zx,s
    __m128 zx_xy_yz=_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r.r),0x27));/// zx,xy,yz,s
    __m128 yz_zx_xy=_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r.r),0x1E));/// yz,zx,xy,s
    __m128 s3=_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r.r),0x00));/// s,s,s,s

    __m128 s=_mm_sub_ps(_mm_sub_ps(_mm_set1_ps(0.5f),_mm_mul_ps(xy_yz_zx,xy_yz_zx)),_mm_mul_ps(zx_xy_yz,zx_xy_yz));

    __m128 p=_mm_add_ps(_mm_mul_ps(yz_zx_xy,xy_yz_zx),_mm_mul_ps(zx_xy_yz,s3));

    __m128 n=_mm_sub_ps(_mm_mul_ps(yz_zx_xy,zx_xy_yz),_mm_mul_ps(xy_yz_zx,s3));

    return(vec3f){.v=_mm_add_ps(_mm_add_ps(_mm_mul_ps(s,v2),
        _mm_mul_ps(p,_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(v2),0xD2)))),  ///zxy w
        _mm_mul_ps(n,_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(v2),0xC9))))}; ///yzx w
}
static inline vec3f r3f_derotate_v3f(rotor3f r,vec3f v)
{
    //__m128 v2=_mm_castsi128_ps(_mm_add_epi32(_mm_castps_si128(v.v),_mm_set1_epi32(0x00800000)));
    /// above would alter zero entries of v to become non-zero, so considered unuseable
    __m128 v2=_mm_mul_ps(v.v,_mm_set1_ps(2.0));

    __m128 xy_yz_zx=_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r.r),0x39));/// xy,yz,zx,s
    __m128 zx_xy_yz=_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r.r),0x27));/// zx,xy,yz,s
    __m128 yz_zx_xy=_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r.r),0x1E));/// yz,zx,xy,s
    __m128 s3=_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r.r),0x00));/// s,s,s,s

    __m128 s=_mm_sub_ps(_mm_sub_ps(_mm_set1_ps(0.5f),_mm_mul_ps(xy_yz_zx,xy_yz_zx)),_mm_mul_ps(zx_xy_yz,zx_xy_yz));

    __m128 p=_mm_add_ps(_mm_mul_ps(yz_zx_xy,zx_xy_yz),_mm_mul_ps(xy_yz_zx,s3));

    __m128 n=_mm_sub_ps(_mm_mul_ps(yz_zx_xy,xy_yz_zx),_mm_mul_ps(zx_xy_yz,s3));

    return(vec3f){.v=_mm_add_ps(_mm_add_ps(_mm_mul_ps(s,v2),
        _mm_mul_ps(p,_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(v2),0xC9)))),  ///yzx w
        _mm_mul_ps(n,_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(v2),0xD2))))}; ///zxy w
}
static inline rotor3f r3f_rotate_around_x_axis(rotor3f r,float a)
{
    float s,c;
    sincosf(a*0.5f,&s,&c);
    return (rotor3f){.r=_mm_add_ps(_mm_mul_ps(_mm_set1_ps(c),r.r),
        _mm_mul_ps(_mm_set_ps(s,s,-s,-s),_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r.r),0x4E))))};
}
static inline rotor3f r3f_rotate_around_y_axis(rotor3f r,float a)
{
    float s,c;
    sincosf(a*0.5f,&s,&c);
    return (rotor3f){.r=_mm_add_ps(_mm_mul_ps(_mm_set1_ps(c),r.r),
        _mm_mul_ps(_mm_set_ps(s,-s,s,-s),_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r.r),0x1B))))};
}
static inline rotor3f r3f_rotate_around_z_axis(rotor3f r,float a)
{
    float s,c;
    sincosf(a*0.5f,&s,&c);
    return (rotor3f){.r=_mm_add_ps(_mm_mul_ps(_mm_set1_ps(c),r.r),
        _mm_mul_ps(_mm_set_ps(-s,s,s,-s),_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r.r),0xB1))))};
}
static inline vec3f r3f_get_x_axis(rotor3f r)
{
    return(vec3f){.v=_mm_mul_ps(_mm_set1_ps(2.0f),_mm_add_ps(_mm_set_ps(0.0f,0.0f,0.0f,0.5f),_mm_add_ps(
        _mm_xor_ps(_mm_set_ps(0.0f,0.0f,0.0f,-0.0f),
            _mm_mul_ps(_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r.r),0x1D)),_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r.r),0x29)))),
        _mm_xor_ps(_mm_set_ps(0.0f,-0.0f,0.0f,-0.0f),
            _mm_mul_ps(_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r.r),0x37)),_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r.r),0x03)))))))};
}
static inline vec3f r3f_get_y_axis(rotor3f r)
{
    return(vec3f){.v=_mm_mul_ps(_mm_set1_ps(2.0f),_mm_add_ps(_mm_set_ps(0.0f,0.0f,0.5f,0.0f),_mm_add_ps(
        _mm_xor_ps(_mm_set_ps(0.0f,0.0f,-0.0f,0.0f),
            _mm_mul_ps(_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r.r),0x16)),_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r.r),0x37)))),
        _mm_xor_ps(_mm_set_ps(0.0f,0.0f,-0.0f,-0.0f),
            _mm_mul_ps(_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r.r),0x28)),_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r.r),0x09)))))))};
}
static inline vec3f r3f_get_z_axis(rotor3f r)
{
    return(vec3f){.v=_mm_mul_ps(_mm_set1_ps(2.0f),_mm_add_ps(_mm_set_ps(0.0f,0.5f,0.0f,0.0f),_mm_add_ps(
        _mm_xor_ps(_mm_set_ps(0.0f,-0.0f,0.0f,0.0f),
            _mm_mul_ps(_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r.r),0x3E)),_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r.r),0x35)))),
        _mm_xor_ps(_mm_set_ps(0.0f,-0.0f,-0.0f,0.0f),
            _mm_mul_ps(_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r.r),0x28)),_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r.r),0x23)))))))};
}
static inline rotor3f r3f_isolate_xy_rotation(rotor3f r)
{
    return (rotor3f){.r=_mm_blend_ps(_mm_set1_ps(0.0f),_mm_div_ps(r.r,_mm_sqrt_ps(_mm_dp_ps(r.r,r.r,0x33))),0x03)};
}
static inline rotor3f r3f_isolate_yz_rotation(rotor3f r)
{
    return (rotor3f){.r=_mm_blend_ps(_mm_set1_ps(0.0f),_mm_div_ps(r.r,_mm_sqrt_ps(_mm_dp_ps(r.r,r.r,0x55))),0x05)};
}
static inline rotor3f r3f_isolate_zx_rotation(rotor3f r)
{
    return (rotor3f){.r=_mm_blend_ps(_mm_set1_ps(0.0f),_mm_div_ps(r.r,_mm_sqrt_ps(_mm_dp_ps(r.r,r.r,0x99))),0x09)};
}
static inline rotor3f r3f_isolate_x_axis_rotation(rotor3f r)
{
    __m128 m,c;

    m=_mm_sqrt_ps(_mm_dp_ps(r.r,r.r,0x5B));/// 0101 1011 -- this handles setting 0 in appropriate place!
    c=_mm_mul_ps(r.r,_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r.r),0x93)));/// s*zx , s*xy , xy*yz , yz*zx 2103
    c=_mm_addsub_ps(c,_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(c),0x0E))); ///  s*zx-xy*yz , s*xy+yz*zx ,?,?  :: need to reshuffle to 0?1? =0x04

    return(rotor3f){.r=_mm_blend_ps(_mm_div_ps(_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(c),0x04)),m),m,0x05)};
}
static inline rotor3f r3f_isolate_y_axis_rotation(rotor3f r)
{
    __m128 m,c;

    m=_mm_sqrt_ps(_mm_dp_ps(r.r,r.r,0x97));/// 1001 0111 -- this handles setting 0 in appropriate place!
    c=_mm_mul_ps(r.r,_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r.r),0x8D)));/// s*xy , xy*zx , yz*s , zx*yz 2031
    c=_mm_addsub_ps(c,_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(c),0x0B))); /// -yz*zx, +s*yz ,?,?  :: need to reshuffle as +/- would be in wrong place... -> ?10? = 10

    return(rotor3f){.r=_mm_blend_ps(_mm_div_ps(_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(c),0x10)),m),m,0x09)};
}
static inline rotor3f r3f_isolate_z_axis_rotation(rotor3f r)
{
    __m128 m,c;

    m=_mm_sqrt_ps(_mm_dp_ps(r.r,r.r,0x3D));/// 0011 1101 -- this handles setting 0 in appropriate place!
    ///on this occasion we can paralellise the shuffles whilst ensuring values end up in the correct place, eliminating a latter shuffle
    c=_mm_mul_ps(_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r.r),0x0E)),_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r.r),0xE5)));/// xy*yz , xy*zx , yz*s , zx*s  0032 3211
    c=_mm_addsub_ps(c,_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(c),0x10))); /// ?,?,-zx*xy, +xy*yz  01 :: no need to reshuffle!

    return(rotor3f){.r=_mm_blend_ps(_mm_div_ps(c,m),m,0x03)};
}
static inline rotor3f r3f_nlerp(rotor3f r1,rotor3f r2,float t)/// normalised linear interpolate
{
    __m128 r=_mm_add_ps(_mm_mul_ps(_mm_set1_ps(t),r2.r),_mm_mul_ps(_mm_set1_ps(1.0f-t),r1.r));
    return (rotor3f){.r=_mm_div_ps(r,_mm_sqrt_ps(_mm_dp_ps(r,r,0xFF)))};
}
static inline rotor3f r3f_bnlerp(rotor3f r0,rotor3f r1,rotor3f r2,rotor3f r3,float t)/// normalised bezier interpolate
{
    ///polynomial expansion of the piecewise factors per imput
    __m128 tf=_mm_add_ps(_mm_mul_ps(_mm_set1_ps(t),_mm_add_ps(_mm_mul_ps(_mm_set1_ps(t),_mm_add_ps(_mm_mul_ps(_mm_set1_ps(t),
        _mm_set_ps(0.5f,-1.5f,1.5f,-0.5f)),_mm_set_ps(-0.5f,2.0f,-2.5f,1.0f))),_mm_set_ps(0.0f,0.5f,0.0f,-0.5f))),_mm_set_ps(0.0f,0.0f,1.0f,0.0f));

    __m128 r=_mm_add_ps(
        _mm_add_ps(
            _mm_mul_ps(_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(tf),0x00)),r0.r),
            _mm_mul_ps(_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(tf),0x55)),r1.r)),
        _mm_add_ps(
            _mm_mul_ps(_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(tf),0xAA)),r2.r),
            _mm_mul_ps(_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(tf),0xFF)),r3.r)));

    return (rotor3f){.r=_mm_div_ps(r,_mm_sqrt_ps(_mm_dp_ps(r,r,0xFF)))};
}
static inline rotor3f r3f_slerp(rotor3f r1,rotor3f r2,float t)/// spherical interpolate
{
    ///dont yet have derivation for this, is basically a length preserving linear interpolation of the 2 elements with the factor scaling by the sin of the angular component
    ///may want to extend to r3f_bezier_spherical_interp <-- this

    float a;
    a=acosf(r3f_dot(r1,r2));
    return (rotor3f){.r=_mm_div_ps(_mm_add_ps(_mm_mul_ps(_mm_set1_ps( sinf((1.0f-t)*a) ),r1.r),_mm_mul_ps(_mm_set1_ps( sinf(t*a) ),r2.r)),_mm_set1_ps(sinf(a)))};
}
static inline rotor3f r3f_bslerp(rotor3f r0,rotor3f r1,rotor3f r2,rotor3f r3,float t)/// bezier spherical interpolate
{
    rotor3f d02,d31;
    float a02,a31,s02,s31,s02f,s31f;

    d02=r3f_mul(r2,r3f_inv(r0));
    d31=r3f_mul(r1,r3f_inv(r3));

    a02=acosf(_mm_cvtss_f32(d02.r));
    a31=acosf(_mm_cvtss_f32(d31.r));

    s02=sinf(a02);
    a02*=0.166666666f;
    s02f=sinf(a02);
    d02.r=_mm_blend_ps(_mm_mul_ps(d02.r,_mm_set1_ps(s02f/s02)),_mm_set_ss(cosf(a02)),0x01);

    s31=sinf(a31);
    a31*=0.166666666f;
    s31f=sinf(a31);
    d31.r=_mm_blend_ps(_mm_mul_ps(d31.r,_mm_set1_ps(s31f/s31)),_mm_set_ss(cosf(a31)),0x01);

    r0=r1;
    r1=r3f_mul(r1,d02);
    r3=r2;
    r2=r3f_mul(r2,d31);

    //r0=r3f_slerp(r0,r1,t);///acos(dp) here is just a02
    r0.r=_mm_div_ps(_mm_add_ps(_mm_mul_ps(_mm_set1_ps( sinf((1.0f-t)*a02) ),r0.r),_mm_mul_ps(_mm_set1_ps( sinf(t*a02) ),r1.r)),_mm_set1_ps(s02f));
    r1=r3f_slerp(r1,r2,t);
//    r2=r3f_slerp(r2,r3,t);///acos(dp) here is just a31
    r2.r=_mm_div_ps(_mm_add_ps(_mm_mul_ps(_mm_set1_ps( sinf((1.0f-t)*a31) ),r2.r),_mm_mul_ps(_mm_set1_ps( sinf(t*a31) ),r3.r)),_mm_set1_ps(s31f));

    r0=r3f_slerp(r0,r1,t);
    r1=r3f_slerp(r1,r2,t);

    return r3f_slerp(r0,r1,t);
}
#else/**================ REFERENCE AND FALLBACK IMPLEMENTATION ================*/

typedef struct vec3i
{
    int32_t x;
    int32_t y;
    int32_t z;
    int32_t dummy_w;
}
vec3i;

///vec3i
static inline vec3i v3i_ini(int32_t x,int32_t y,int32_t z)
{
    return (vec3i){.x=x,.y=y,.z=z};
}
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
static inline vec3i v3i_mod(vec3i v,int32_t c)
{
    return (vec3i){.x=v.x%c,.y=v.y%c,.z=v.z%c};
}
static inline vec3i v3i_rsh(vec3i v,int32_t c)
{
    return (vec3i){.x=v.x>>c,.y=v.y>>c,.z=v.z>>c};
}
static inline vec3i v3i_lsh(vec3i v,int32_t c)
{
    return (vec3i){.x=v.x<<c,.y=v.y<<c,.z=v.z<<c};
}
static inline vec3i v3i_min(vec3i v1,vec3i v2)
{
//    return (vec3i){ .x= v1.x+(((v2.x-v1.x)>>31)&(v2.x-v1.x)),
//                    .y= v1.y+(((v2.y-v1.y)>>31)&(v2.y-v1.y)),
//                    .z= v1.z+(((v2.z-v1.z)>>31)&(v2.z-v1.z))};
    return (vec3i){.x=(v1.x<v2.x)?v1.x:v2.x,.y=(v1.y<v2.y)?v1.y:v2.y,.z=(v1.z<v2.z)?v1.z:v2.z};
}
static inline vec3i v3i_max(vec3i v1,vec3i v2)
{
//    return (vec3i){ .x= v1.x-(((v1.x-v2.x)>>31)&(v1.x-v2.x)),
//                    .y= v1.y-(((v1.y-v2.y)>>31)&(v1.y-v2.y)),
//                    .z= v1.z-(((v1.z-v2.z)>>31)&(v1.z-v2.z))};
    return (vec3i){.x=(v1.x>v2.x)?v1.x:v2.x,.y=(v1.y>v2.y)?v1.y:v2.y,.z=(v1.z>v2.z)?v1.z:v2.z};
}
static inline vec3i v3i_clamp(vec3i v,vec3i min,vec3i max)
{
//    return (vec3i){ .x= v.x-(((v.x-min.x)>>31)&(v.x-min.x))+(((max.x-v.x)>>31)&(max.x-v.x)),
//                    .y= v.y-(((v.y-min.y)>>31)&(v.y-min.y))+(((max.y-v.y)>>31)&(max.y-v.y)),
//                    .z= v.z-(((v.z-min.z)>>31)&(v.z-min.z))+(((max.z-v.z)>>31)&(max.z-v.z))};
    return (vec3i){.x=(v.x<min.x)?min.x:(v.x>max.x)?max.x:v.x,.y=(v.y<min.y)?min.y:(v.y>max.y)?max.y:v.y,.z=(v.z<min.z)?min.z:(v.z>max.z)?max.z:v.z};
}
static inline vec3i v3i_clamp_range(vec3i v,int32_t min,int32_t max)
{
//    return (vec3i){ .x= v.x-(((v.x-min)>>31)&(v.x-min))+(((max-v.x)>>31)&(max-v.x)),
//                    .y= v.y-(((v.y-min)>>31)&(v.y-min))+(((max-v.y)>>31)&(max-v.y)),
//                    .z= v.z-(((v.z-min)>>31)&(v.z-min))+(((max-v.z)>>31)&(max-v.z))};
    return (vec3i){.x=(v.x<min)?min:(v.x>max)?max:v.x,.y=(v.y<min)?min:(v.y>max)?max:v.y,.z=(v.z<min)?min:(v.z>max)?max:v.z};
}





typedef struct vec3f
{
    float x;
    float y;
    float z;
    float dummy_w;
}
vec3f;

///vec3f
static inline vec3f v3f_ini(float x,float y,float z)
{
    return (vec3f){.x=x,.y=y,.z=z};
}
static inline vec3f v3f_add(vec3f v1,vec3f v2)
{
    return (vec3f){.x=v1.x+v2.x,.y=v1.y+v2.y,.z=v1.z+v2.z};
}
static inline vec3f v3f_sub(vec3f v1,vec3f v2)
{
    return (vec3f){.x=v1.x-v2.x,.y=v1.y-v2.y,.z=v1.z-v2.z};
}
static inline vec3f v3f_mul(vec3f v,float c)
{
    return (vec3f){.x=v.x*c,.y=v.y*c,.z=v.z*c};
}
static inline vec3f v3f_div(vec3f v,float c)
{
    return (vec3f){.x=v.x/c,.y=v.y/c,.z=v.z/c};
}
static inline float v3f_dot(vec3f v1,vec3f v2)
{
    return v1.x*v2.x+v1.y*v2.y+v1.z*v2.z;
}
static inline float v3f_mag_sqr(vec3f v)
{
    return v3f_dot(v,v);
}
static inline float v3f_mag(vec3f v)
{
    return sqrtf(v3f_mag_sqr(v));
}
static inline vec3f v3f_nrm(vec3f v)
{
    return v3f_mul(v,1.0f/v3f_mag(v));
}
static inline vec3f v3f_nrm_fst(vec3f v)///not relevant w/o sse
{
    return v3f_nrm(v);
}
static inline float v3f_dst_sqr(vec3f v1,vec3f v2)
{
    return v3f_mag_sqr(v3f_sub(v1,v2));
}
static inline float v3f_dst(vec3f v1,vec3f v2)
{
    return v3f_mag(v3f_sub(v1,v2));
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
static inline vec3f v3f_cross(vec3f v1,vec3f v2)
{
    return (vec3f){.x=v1.y*v2.z - v1.z*v2.y,.y=v1.z*v2.x - v1.x*v2.z,.z=v1.x*v2.y - v1.y*v2.x};
}
static inline vec3f v3f_rotate(vec3f v,vec3f k,float cos_theta,float sin_theta)///Rodrigues rotation
{
    float d=(k.x*v.x+k.y*v.y+k.z*v.z)*(1.0f-cos_theta);

    return (vec3f){ .x= v.x*cos_theta  +  (k.y*v.z - k.z*v.y)*sin_theta  +  k.x*d,
                    .y= v.y*cos_theta  +  (k.z*v.x - k.x*v.z)*sin_theta  +  k.y*d,
                    .z= v.z*cos_theta  +  (k.x*v.y - k.y*v.x)*sin_theta  +  k.z*d};
}
static inline vec3f v3f_from_spherical_direction(float zenith,float azimuth)
{
    float sz,cz,sa,ca;
    sincosf(zenith,&sz,&cz);
    sincosf(azimuth,&sa,&ca);
    return (vec3f){ .x=ca*sz,.y=sa*sz,.z=cz};
}
static inline vec3f v3f_from_spherical(float r,float zenith,float azimuth)
{
    return v3f_mul(v3f_from_spherical_direction(zenith,azimuth),r);
}




typedef struct vec4f
{
    float x;
    float y;
    float z;
    float w;
}
vec4f;

///vec4f
static inline vec4f v4f_ini(float x,float y,float z,float w)
{
    return (vec4f){.x=x,.y=y,.z=z,.w=w};
}
static inline vec4f v4f_add(vec4f v1,vec4f v2)
{
    return (vec4f){.x=v1.x+v2.x,.y=v1.y+v2.y,.z=v1.z+v2.z,.w=v1.w+v2.w};
}
static inline vec4f v4f_sub(vec4f v1,vec4f v2)
{
    return (vec4f){.x=v1.x+v2.x,.y=v1.y+v2.y,.z=v1.z+v2.z,.w=v1.w+v2.w};
}
static inline vec4f v4f_mul(vec4f v,float c)
{
    return (vec4f){.x=v.x*c,.y=v.y*c,.z=v.z*c,.w=v.w*c};
}
static inline vec4f v4f_div(vec4f v,float c)
{
    return (vec4f){.x=v.x/c,.y=v.y/c,.z=v.z/c,.w=v.w/c};
}
static inline float v4f_dot(vec4f v1,vec4f v2)
{
    return v1.x*v2.x+v1.y*v2.y+v1.z*v2.z+v1.w*v2.w;
}
static inline float v4f_mag_sqr(vec4f v)
{
    return v4f_dot(v,v);
}
static inline float v4f_mag(vec4f v)
{
    return sqrtf(v4f_dot(v,v));
}
static inline vec4f v4f_nrm(vec4f v)
{
    return v4f_mul(v,1.0f/v4f_mag(v));
}
static inline vec4f v4f_nrm_fst(vec4f v)///fast (but more inaccurate) variant (only true if using sse or similar intrinsic set)
{
    return v4f_nrm(v);
}
static inline float v4f_dst_sqr(vec4f v1,vec4f v2)
{
    return v4f_mag_sqr(v4f_sub(v1,v2));
}
static inline float v4f_dst(vec4f v1,vec4f v2)
{
    return v4f_mag(v4f_sub(v1,v2));
}
static inline vec4f v4f_max(vec4f v1,vec4f v2)
{
    return (vec4f){.x=(v1.x>v2.x)?v1.x:v2.x,.y=(v1.y>v2.y)?v1.y:v2.y,.z=(v1.z>v2.z)?v1.z:v2.z,.w=(v1.w>v2.w)?v1.w:v2.w};
}
static inline vec4f v4f_min(vec4f v1,vec4f v2)
{
    return (vec4f){.x=(v1.x<v2.x)?v1.x:v2.x,.y=(v1.y<v2.y)?v1.y:v2.y,.z=(v1.z<v2.z)?v1.z:v2.z,.w=(v1.w<v2.w)?v1.w:v2.w};
}
static inline vec4f v4f_clamp_range(vec4f v,float min,float max)
{
    return (vec4f){ .x=(v.x<min)?min:(v.x>max)?max:v.x,
                    .y=(v.y<min)?min:(v.y>max)?max:v.y,
                    .z=(v.z<min)?min:(v.z>max)?max:v.z,
                    .w=(v.w<min)?min:(v.w>max)?max:v.w};
}
static inline vec4f vec4f_blend(vec4f b,vec4f f)///treats w as alpha
{
    return (vec4f){.x=(1.0f-f.w)*b.x+f.w*f.x,.y=(1.0f-f.w)*b.y+f.w*f.y,.z=(1.0f-f.w)*b.z+f.w*f.z,.w=(1.0f-f.w)*b.w+f.w};
}



typedef struct plane
{
    float x;
    float y;
    float z;
    float dist;
}
plane;

///plane
static inline plane plane_from_normal_and_point(vec3f n,vec3f p)
{
    return (plane){.x=n.x,.y=n.y,.z=n.z,.dist=v3f_dot(n,p)};
}
static inline float plane_point_dist(plane p,vec3f point)
{
    return p.x*point.x + p.y*point.y + p.z*point.z - p.dist;
}



typedef struct mat44f
{
    vec4f x;
    vec4f y;
    vec4f z;
    vec4f w;
}
mat44f;

///mat44f
static inline mat44f m44f_transpose(mat44f m)
{
    mat44f r;

    r.x.x=m.x.x;
    r.x.y=m.y.x;
    r.x.z=m.z.x;
    r.x.w=m.w.x;

    r.y.x=m.x.y;
    r.y.y=m.y.y;
    r.y.z=m.z.y;
    r.y.w=m.w.y;

    r.z.x=m.x.z;
    r.z.y=m.y.z;
    r.z.z=m.z.z;
    r.z.w=m.w.z;

    r.w.x=m.x.w;
    r.w.y=m.y.w;
    r.w.z=m.z.w;
    r.w.w=m.w.w;

    return r;
}
static inline vec4f m44f_v4f_mul(mat44f m,vec4f v)
{
    return (vec4f){ .x=m.x.x*v.x + m.y.x*v.y + m.z.x*v.z + m.w.x*v.w,
                    .y=m.x.y*v.x + m.y.y*v.y + m.z.y*v.z + m.w.y*v.w,
                    .z=m.x.z*v.x + m.y.z*v.y + m.z.z*v.z + m.w.z*v.w,
                    .w=m.x.w*v.x + m.y.w*v.y + m.z.w*v.z + m.w.w*v.w};
}
static inline vec3f m44f_v4f_mul_p(mat44f m,vec4f v)
{
    return v3f_div(
    (vec3f){.x=m.x.x*v.x + m.y.x*v.y + m.z.x*v.z + m.w.x*v.w,
            .y=m.x.y*v.x + m.y.y*v.y + m.z.y*v.z + m.w.y*v.w,
            .z=m.x.z*v.x + m.y.z*v.y + m.z.z*v.z + m.w.z*v.w},
               m.x.w*v.x + m.y.w*v.y + m.z.w*v.z + m.w.w*v.w);
}



typedef struct mat33f
{
    vec3f x;
    vec3f y;
    vec3f z;
}
mat33f;

static inline mat33f m33f_transpose(mat33f m)
{
    float t;

    t=m.x.y;
    m.x.y=m.y.x;
    m.y.x=t;

    t=m.x.z;
    m.x.z=m.z.x;
    m.z.x=t;

    t=m.y.z;
    m.y.z=m.z.y;
    m.z.y=t;

    return m;
}
static inline vec3f m33f_v3f_mul(mat33f m,vec3f v)
{
    return (vec3f)
    {
        .x=m.x.x*v.x + m.y.x*v.y + m.z.x*v.z,
        .y=m.x.y*v.x + m.y.y*v.y + m.z.y*v.z,
        .z=m.x.z*v.x + m.y.z*v.y + m.z.z*v.z
    };
}



typedef struct rotor3f
{
    float s;///scalar / symmetric component (dot product in geometric product)
    float xy;
    float yz;
    float zx;
}
rotor3f;

///rotor3f
static inline rotor3f r3f_ini(float s,float xy,float yz,float zx)
{
    return(rotor3f){.s=s,.xy=xy,.yz=yz,.zx=zx};
}
static inline float r3f_dot(rotor3f r1,rotor3f r2)
{
    return r1.s*r2.s+r1.xy*r2.xy+r1.yz*r2.yz+r1.zx*r2.zx;
}
static inline rotor3f r3f_nrm(rotor3f r)///useful for ensuring object rotation doesn't start introducing skew as error accumulates over time
{
    float im=1.0f/sqrtf(r.s*r.s+r.xy*r.xy+r.yz*r.yz+r.zx*r.zx);
    return(rotor3f){.s=r.s*im,.xy=r.xy*im,.yz=r.yz*im,.zx=r.zx*im};
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
    m=sqrt(v1.x*v1.x+v1.y*v1.y+v1.z*v1.z + v2.x*v2.x+v2.y*v2.y+v2.z*v2.z  + 2*(v1.x*v2.x+v1.y*v2.y+2*v1.z*v2.z)); dot(v1,v1)=dot(v2,v2)=1.0f
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
    float d1=1.0 + v1.x*v2.x + v1.y*v2.y + v1.z*v2.z;//1.0f+d
    float im=SQRT_HALF/sqrtf(d1);///inverse_magnitude
    ///1.0f/sqrt(2.0f+2.0f*d) == sqrt_half/sqrt(1+d)

    return(rotor3f)
    {
        .s=im*d1,///(1+d)*im = d1 *im
        .xy=im*(v1.x*v2.y-v1.y*v2.x),
        .yz=im*(v1.y*v2.z-v1.z*v2.y),
        .zx=im*(v1.z*v2.x-v1.x*v2.z)
    };
}
static inline rotor3f r3f_from_angle(vec3f v,float a)
{
    ///assumes input vector is normalised
    float s,c;
    sincosf(a*0.5f,&s,&c);
    return(rotor3f){.s=c,.xy=s*v.z,.yz=s*v.x,.zx=s*v.y};
}
///in terms of applying rotations, LHS gets applied first
static inline rotor3f r3f_mul(rotor3f r1,rotor3f r2)
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
static inline rotor3f r3f_inv(rotor3f r)
{
    return(rotor3f){.s=r.s,.xy=-r.xy,.yz=-r.yz,.zx=-r.zx};
}
static inline mat33f r3f_to_m33f(rotor3f r)
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

	// now substite a,b,c = xy,yz,zx for rotor and f,g,h = x,y,z for vector and put in matrix form
	// also note every rotor component is multiplied by another which are in turn multiplied by 2
	//   ^ ergo we can premultiply all components by sqrt 2 to avoid unnecessary multiplications
    */

    r.s*=(float)SQRT_2;
    r.xy*=(float)SQRT_2;
    r.yz*=(float)SQRT_2;
    r.zx*=(float)SQRT_2;

    return (mat33f)
    {
        .x=
        {
            .x=1.0f - r.xy*r.xy - r.zx*r.zx,
            .y=r.zx*r.yz + r.xy*r.s,
            .z=r.xy*r.yz - r.zx*r.s
        },
        .y=
        {
            .x=r.yz*r.zx - r.s*r.xy,
            .y=1.0f - r.xy*r.xy - r.yz*r.yz,
            .z=r.xy*r.zx + r.yz*r.s
        },
        .z=
        {
            .x=r.yz*r.xy + r.s*r.zx,
            .y=r.zx*r.xy - r.yz*r.s,
            .z=1.0f - r.zx*r.zx - r.yz*r.yz
        }
    };
}
static inline vec3f r3f_rotate_v3f(rotor3f r,vec3f v)
{
    return(vec3f)
    {
        .x=2.0f*(v.x*(0.5f - r.xy*r.xy - r.zx*r.zx) + v.y*(r.yz*r.zx - r.s*r.xy) + v.z*(r.yz*r.xy + r.s*r.zx)),
        .y=2.0f*(v.x*(r.zx*r.yz + r.xy*r.s) + v.y*(0.5f - r.xy*r.xy - r.yz*r.yz) + v.z*(r.zx*r.xy - r.yz*r.s)),
        .z=2.0f*(v.x*(r.xy*r.yz - r.zx*r.s) + v.y*(r.xy*r.zx + r.yz*r.s) + v.z*(0.5f - r.zx*r.zx - r.yz*r.yz))
    };
}
static inline vec3f r3f_derotate_v3f(rotor3f r,vec3f v)
{
    /// reverse the rotation that would be performed by r, can be figured out based on rotation operation of r on v (as in r3f_rotate_v3f)
    /// that rotation being represented/thought of as an orthonormal basis space transformation (a matrix)
    /// and the inverse of an orthonormal matrix is just its that matrix transposed!
    return(vec3f)
    {
        .x=2.0f*(v.x*(0.5f - r.xy*r.xy - r.zx*r.zx) + v.y*(r.zx*r.yz + r.xy*r.s) + v.z*(r.xy*r.yz - r.zx*r.s)),
        .y=2.0f*(v.x*(r.yz*r.zx - r.s*r.xy) + v.y*(0.5f - r.xy*r.xy - r.yz*r.yz) + v.z*(r.xy*r.zx + r.yz*r.s)),
        .z=2.0f*(v.x*(r.yz*r.xy + r.s*r.zx) + v.y*(r.zx*r.xy - r.yz*r.s) + v.z*(0.5f - r.zx*r.zx - r.yz*r.yz))
    };
}
static inline rotor3f r3f_rotate_around_x_axis(rotor3f r,float a)
{
    float s,c;
    sincosf(a*0.5f,&s,&c);
    return (rotor3f){.s=c*r.s-s*r.yz, .xy=c*r.xy-s*r.zx, .yz=c*r.yz+s*r.s, .zx=c*r.zx+s*r.xy};
}
static inline rotor3f r3f_rotate_around_y_axis(rotor3f r,float a)
{
    float s,c;
    sincosf(a*0.5f,&s,&c);
    return (rotor3f){.s=c*r.s-s*r.zx, .xy=c*r.xy+s*r.yz, .yz=c*r.yz-s*r.xy, .zx=c*r.zx+s*r.s};
}
static inline rotor3f r3f_rotate_around_z_axis(rotor3f r,float a)
{
    float s,c;
    sincosf(a*0.5f,&s,&c);
    return (rotor3f){.s=c*r.s-s*r.xy, .xy=c*r.xy+s*r.s, .yz=c*r.yz+s*r.zx, .zx=c*r.zx-s*r.yz};
}
static inline vec3f r3f_get_x_axis(rotor3f r)
{
    return(vec3f){.x=1.0f-2.0f*(r.xy*r.xy + r.zx*r.zx),.y=2.0f*(r.zx*r.yz + r.xy*r.s),.z=2.0f*(r.xy*r.yz - r.zx*r.s)};
}
static inline vec3f r3f_get_y_axis(rotor3f r)
{
    return(vec3f){.x=2.0f*(r.yz*r.zx - r.s*r.xy),.y=1.0f-2.0f*(r.xy*r.xy + r.yz*r.yz),.z=2.0f*(r.xy*r.zx + r.yz*r.s)};
}
static inline vec3f r3f_get_z_axis(rotor3f r)
{
    return(vec3f){.x=2.0f*(r.yz*r.xy + r.s*r.zx),.y=2.0f*(r.zx*r.xy - r.yz*r.s),.z=1.0f-2.0f*(r.zx*r.zx + r.yz*r.yz)};
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
      =0.5f/sqrt(1-dd-cc) // both this and following will be useful, use whichever is most useful for given situation
      =0.5f/sqrt(aa+bb) // aa+bb+cc+dd = 1   therefore   1-cc-dd = aa+bb

    z_rot=

    .s = (1+dot_product)*im
       = (2-2*dd-2*cc)*0.5f/sqrt(1-dd-cc)
       = (1-dd-cc)/sqrt(1-dd-cc)
       = sqrt(1-dd-cc)
       = sqrt(aa+bb) // aa+bb+cc+dd = 1   therefore   1-cc-dd = aa+bb

    .xy = 0 // no v1.z, makes sense as we're avoiding this component

    .yz = -v2.y * im
        = -2(db-ac) * 0.5f/sqrt(aa+bb)
        = (ac-db)/sqrt(aa+bb)

    .zx = v2.x * im
        = 2(cb+ad) * 0.5f/sqrt(aa+bb)
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

    float im=1.0f/sqrtf(r.s*r.s+r.xy*r.xy);///inverse magnitude
    return(rotor3f){.s=r.s*im,.xy=r.xy*im,.yz=0.0,.zx=0.0};
}
static inline rotor3f r3f_isolate_yz_rotation(rotor3f r)/// isolates the rotation about the x axis
{
    /// logic/maths the same as in r3f_isolate_xy_rotation, just for different bivector

    float im=1.0f/sqrtf(r.s*r.s+r.yz*r.yz);///inverse magnitude
    return(rotor3f){.s=r.s*im,.xy=0.0,.yz=r.yz*im,.zx=0.0};
}
static inline rotor3f r3f_isolate_zx_rotation(rotor3f r)/// isolates the rotation about the y axis
{
    /// logic/maths the same as in r3f_isolate_xy_rotation, just for different bivector

    float im=1.0f/sqrtf(r.s*r.s+r.zx*r.zx);///inverse magnitude
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
      =0.5f/sqrt(1-bb-dd) // both this and following will be useful, use whichever is most useful for given situation
      =0.5f/sqrt(aa+cc) // aa+bb+cc+dd = 1   therefore   1-cc-bb = aa+dd


    z_rot=

    .s = (1+dot_product)*im
       = (2-2*bb-2*dd)*0.5f/sqrt(1-bb-dd)
       = (1-bb-dd)/sqrt(1-bb-dd)
       = sqrt(1-bb-dd)
       = sqrt(aa+cc) // aa+bb+cc+dd = 1   therefore   1-cc-bb = aa+dd

    .xy = v2.y * im
        = 2(cd+ab) * 0.5f/sqrt(aa+cc)
        = (cd+ab)/sqrt(aa+cc)

    .yz = 0 // no v1.y, makes sense as we're avoiding this component

    .zx = -v2.z * im
        = 2(ad-bc) * 0.5f/sqrt(aa+cc)
        = (ad-bc)/sqrt(aa+cc)
    */

    float m=sqrtf(r.s*r.s+r.yz*r.yz);
    float im=1.0f/m;///inverse magnitude
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
      =0.5f/sqrt(1-bb-cc) // both this and following will be useful, use whichever is most useful for given situation
      =0.5f/sqrt(aa+dd) // aa+bb+cc+dd = 1   therefore   1-cc-bb = aa+dd


    z_rot=

    .s = (1+dot_product)*im
       = (2-2*bb-2*cc)*0.5f/sqrt(1-bb-cc)
       = (1-bb-cc)/sqrt(1-bb-cc)
       = sqrt(1-bb-cc)
       = sqrt(aa+dd) // aa+bb+cc+dd = 1   therefore   1-cc-bb = aa+dd

    .xy = -v2.x * im
        = -2(cd-ab) * 0.5f/sqrt(aa+dd)
        = (ab-cd)/sqrt(aa+dd)

    .yz = v2.z * im
        = 2(bd+ac) * 0.5f/sqrt(aa+dd)
        = (bd+ac)/sqrt(aa+dd)

    .zx = 0 // no v1.y, makes sense as we're avoiding this component
    */

    float m=sqrtf(r.s*r.s+r.zx*r.zx);
    float im=1.0f/m;///inverse magnitude
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
      =0.5f/sqrt(1-dd-cc) // both this and following will be useful, use whichever is most useful for given situation
      =0.5f/sqrt(aa+bb) // aa+bb+cc+dd = 1   therefore   1-cc-dd = aa+bb


    z_rot=

    .s = (1+dot_product)*im
       = (2-2*dd-2*cc)*0.5f/sqrt(1-dd-cc)
       = (1-dd-cc)/sqrt(1-dd-cc)
       = sqrt(1-dd-cc)r3f_bnlerp
       = sqrt(aa+bb) // aa+bb+cc+dd = 1   therefore   1-cc-dd = aa+bb

    .xy = 0 // no v1.z, makes sense as we're avoiding this component

    .yz = -v2.y * im
        = -2(db-ac) * 0.5f/sqrt(aa+bb)
        = (ac-db)/sqrt(aa+bb)

    .zx = v2.x * im
        = 2(cb+ad) * 0.5f/sqrt(aa+bb)
        = (cb+ad)/sqrt(aa+bb)
    */

    float m=sqrtf(r.s*r.s+r.xy*r.xy);
    float im=1.0f/m;///inverse magnitude
    return(rotor3f){.s=m,.xy=0.0,.yz=(r.s*r.yz-r.zx*r.xy)*im,.zx=(r.xy*r.yz +r.s*r.zx)*im};
}
static inline rotor3f r3f_nlerp(rotor3f r1,rotor3f r2,float t)///normalised linear interpolate
{
    float u;
    u=1.0f-t;

    r1.s =r1.s *u+r2.s *t;
    r1.xy=r1.xy*u+r2.xy*t;
    r1.yz=r1.yz*u+r2.yz*t;
    r1.zx=r1.zx*u+r2.zx*t;

    u=1.0f/sqrtf(r1.s*r1.s+r1.xy*r1.xy+r1.yz*r1.yz+r1.zx*r1.zx);
    r1.s*=u;
    r1.xy*=u;
    r1.yz*=u;
    r1.zx*=u;

    return r1;
}
static inline rotor3f r3f_bnlerp(rotor3f r0,rotor3f r1,rotor3f r2,rotor3f r3,float t)/// normalised bezier interpolate
{
    float u,uut,utt;

    u=1.0f-t;
    uut=u*u*t*3.0f;
    utt=u*t*t*3.0f;
    t*=t*t;
    u*=u*u;

    r1.s =r1.s *u+(r1.s +(r2.s -r0.s )*0.166666666f)*uut+(r2.s +(r1.s -r3.s )*0.166666666f)*utt+r2.s *t;
    r1.xy=r1.xy*u+(r1.xy+(r2.xy-r0.xy)*0.166666666f)*uut+(r2.xy+(r1.xy-r3.xy)*0.166666666f)*utt+r2.xy*t;
    r1.yz=r1.yz*u+(r1.yz+(r2.yz-r0.yz)*0.166666666f)*uut+(r2.yz+(r1.yz-r3.yz)*0.166666666f)*utt+r2.yz*t;
    r1.zx=r1.zx*u+(r1.zx+(r2.zx-r0.zx)*0.166666666f)*uut+(r2.zx+(r1.zx-r3.zx)*0.166666666f)*utt+r2.zx*t;

    u=1.0f/sqrtf(r1.s*r1.s+r1.xy*r1.xy+r1.yz*r1.yz+r1.zx*r1.zx);
    r1.s*=u;
    r1.xy*=u;
    r1.yz*=u;
    r1.zx*=u;

    return r1;
}
static inline rotor3f r3f_slerp(rotor3f r1,rotor3f r2,float t)/// spherical interpolate
{
    float u,a,d;

    a=acosf(r3f_dot(r1,r2));
    d=1.0f/sinf(a);

    u=sinf((1.0f-t)*a)*d;
    t=sinf(t*a)*d;

    r1.s =r1.s *u+r2.s *t;
    r1.xy=r1.xy*u+r2.xy*t;
    r1.yz=r1.yz*u+r2.yz*t;
    r1.zx=r1.zx*u+r2.zx*t;

    return r1;
}
static inline rotor3f r3f_bslerp(rotor3f r0,rotor3f r1,rotor3f r2,rotor3f r3,float t)/// bezier spherical interpolate
{
    rotor3f d02,d31;
    float a02,a31,s02,s31,s02f,s31f,u,d;

    d02=r3f_mul(r2,r3f_inv(r0));
    d31=r3f_mul(r1,r3f_inv(r3));

    a02=acosf(d02.s);
    a31=acosf(d31.s);

    s02=sinf(a02);
    a02*=0.166666666f;
    s02f=sinf(a02);
    s02=s02f/s02;
    d02.s=cosf(a02);
    d02.xy*=s02;
    d02.yz*=s02;
    d02.zx*=s02;

    s31=sinf(a31);
    a31*=0.166666666f;
    s31f=sinf(a31);
    s31=s31f/s31;
    d31.s=cosf(a31);
    d31.xy*=s31;
    d31.yz*=s31;
    d31.zx*=s31;

    r0=r1;
    r1=r3f_mul(r1,d02);
    r3=r2;
    r2=r3f_mul(r2,d31);

    //r0=r3f_slerp(r0,r1,t);///acos(dp) here is just a02
    d=1.0f/s02f;
    u=sinf((1.0f-t)*a02)*d;
    t=sinf(t*a02)*d;
    r0.s =r0.s *u+r1.s *t;
    r0.xy=r0.xy*u+r1.xy*t;
    r0.yz=r0.yz*u+r1.yz*t;
    r0.zx=r0.zx*u+r1.zx*t;

    r1=r3f_slerp(r1,r2,t);

//    r2=r3f_slerp(r2,r3,t);///acos(dp) here is just a31
    d=1.0f/s31f;
    u=sinf((1.0f-t)*a31)*d;
    t=sinf(t*a31)*d;
    r2.s =r2.s *u+r3.s *t;
    r2.xy=r2.xy*u+r3.xy*t;
    r2.yz=r2.yz*u+r3.yz*t;
    r2.zx=r2.zx*u+r3.zx*t;

    r0=r3f_slerp(r0,r1,t);
    r1=r3f_slerp(r1,r2,t);

    return r3f_slerp(r0,r1,t);
}
#endif









static inline float m3f_det(mat33f m)
{
    assert(false);///needs to be vonverted to vector/intrinsic ops, though really would to prefer not using m3x3S
    return m.x.x*m.y.y*m.z.z + m.y.x*m.z.y*m.x.z + m.z.x*m.x.y*m.y.z - m.z.x*m.y.y*m.x.z - m.y.x*m.x.y*m.z.z - m.x.x*m.z.y*m.y.z;
}


mat44f m44f_mul(mat44f l,mat44f r);
mat44f m44f_inv(mat44f m);

mat33f m33f_mul(mat33f l,mat33f r);
mat33f m33f_inv(mat33f m);
mat33f m33f_inv_det(mat33f m,float * determinant);


void m44f_print(mat44f m);
void m33f_print(mat33f m);


#endif

