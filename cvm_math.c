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

#include "solipsix.h"




//#define CVM_INTRINSIC_MODE_NONE
//#define CVM_INTRINSIC_MODE_SSE /*this can trick IDE, when it doesn't pick up __SSE__ :p*/


///start with mmult, also probably want to rename, first thing is mat3x3 ??

#if (defined __SSE__ || defined CVM_INTRINSIC_MODE_SSE) && !defined CVM_INTRINSIC_MODE_NONE
mat44f m44f_mul(mat44f l,mat44f r)
{
    r.x.v=_mm_add_ps(
        _mm_add_ps(
            _mm_mul_ps(l.x.v,_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r.x.v),0x00))),
            _mm_mul_ps(l.y.v,_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r.x.v),0x55)))),
        _mm_add_ps(
            _mm_mul_ps(l.z.v,_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r.x.v),0xAA))),
            _mm_mul_ps(l.w.v,_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r.x.v),0xFF)))));

    r.y.v=_mm_add_ps(
        _mm_add_ps(
            _mm_mul_ps(l.x.v,_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r.y.v),0x00))),
            _mm_mul_ps(l.y.v,_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r.y.v),0x55)))),
        _mm_add_ps(
            _mm_mul_ps(l.z.v,_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r.y.v),0xAA))),
            _mm_mul_ps(l.w.v,_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r.y.v),0xFF)))));

    r.z.v=_mm_add_ps(
        _mm_add_ps(
            _mm_mul_ps(l.x.v,_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r.z.v),0x00))),
            _mm_mul_ps(l.y.v,_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r.z.v),0x55)))),
        _mm_add_ps(
            _mm_mul_ps(l.z.v,_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r.z.v),0xAA))),
            _mm_mul_ps(l.w.v,_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r.z.v),0xFF)))));

    r.w.v=_mm_add_ps(
        _mm_add_ps(
            _mm_mul_ps(l.x.v,_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r.w.v),0x00))),
            _mm_mul_ps(l.y.v,_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r.w.v),0x55)))),
        _mm_add_ps(
            _mm_mul_ps(l.z.v,_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r.w.v),0xAA))),
            _mm_mul_ps(l.w.v,_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r.w.v),0xFF)))));

    return r;
}
mat44f m44f_inv(mat44f m)
{
    mat44f inv;

    __m128 p1=_mm_sub_ps(_mm_mul_ps(_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.z.v),0x9E)),_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.w.v),0x7B))),
                         _mm_mul_ps(_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.z.v),0x7B)),_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.w.v),0x9E))));
    __m128 p2=_mm_sub_ps(_mm_mul_ps(_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.z.v),0x33)),_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.w.v),0x8D))),
                         _mm_mul_ps(_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.z.v),0x8D)),_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.w.v),0x33))));
    __m128 p3=_mm_sub_ps(_mm_mul_ps(_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.z.v),0x49)),_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.w.v),0x12))),
                         _mm_mul_ps(_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.z.v),0x12)),_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.w.v),0x49))));

    inv.x.v=_mm_add_ps(_mm_add_ps(
        _mm_mul_ps(_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.y.v),0x01)),p1),   // yxxx 0001
        _mm_mul_ps(_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.y.v),0x5A)),p2)),  // zzyy 1122
        _mm_mul_ps(_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.y.v),0xBF)),p3));  // wwwz 2333

    __m128 x_yxxx=_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.x.v),0x01));
    __m128 x_zzyy=_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.x.v),0x5A));
    __m128 x_wwwz=_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.x.v),0xBF));

    ///can just negate this whole thing! (using same guts)
    ///_mm_xor_si128 < this, flip sign bits
    inv.y.v=_mm_castsi128_ps(_mm_xor_si128(_mm_set1_epi32(0x80000000),_mm_castps_si128(
        _mm_add_ps(_mm_add_ps(_mm_mul_ps(x_yxxx,p1),_mm_mul_ps(x_zzyy,p2)),_mm_mul_ps(x_wwwz,p3)))));


    inv.z.v=_mm_add_ps(_mm_add_ps(
        _mm_mul_ps(x_yxxx,_mm_sub_ps(
                _mm_mul_ps(_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.y.v),0x9E)),_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.w.v),0x7B))),
                _mm_mul_ps(_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.y.v),0x7B)),_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.w.v),0x9E))))),
        _mm_mul_ps(x_zzyy,_mm_sub_ps(
                _mm_mul_ps(_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.y.v),0x33)),_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.w.v),0x8D))),
                _mm_mul_ps(_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.y.v),0x8D)),_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.w.v),0x33)))))),
        _mm_mul_ps(x_wwwz,
            _mm_sub_ps(
                _mm_mul_ps(_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.y.v),0x49)),_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.w.v),0x12))),
                _mm_mul_ps(_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.y.v),0x12)),_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.w.v),0x49))))));


    inv.w.v=_mm_add_ps(_mm_add_ps(
        _mm_mul_ps(x_yxxx,_mm_sub_ps(
                _mm_mul_ps(_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.y.v),0x7B)),_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.z.v),0x9E))),
                _mm_mul_ps(_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.y.v),0x9E)),_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.z.v),0x7B))))),
        _mm_mul_ps(x_zzyy,_mm_sub_ps(
                _mm_mul_ps(_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.y.v),0x8D)),_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.z.v),0x33))),
                _mm_mul_ps(_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.y.v),0x33)),_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.z.v),0x8D)))))),
        _mm_mul_ps(x_wwwz,_mm_sub_ps(
                _mm_mul_ps(_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.y.v),0x12)),_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.z.v),0x49))),
                _mm_mul_ps(_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.y.v),0x49)),_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.z.v),0x12))))));


    __m128 inv_det=_mm_dp_ps(m.x.v,inv.x.v,0xFF);
    assert(_mm_cvtss_f32(inv_det)!=0.0f);///matrix is not invertible
    inv_det=_mm_div_ps(_mm_set1_ps(1.0f),inv_det);

    inv.x.v=_mm_mul_ps(inv.x.v,inv_det);
    inv.y.v=_mm_mul_ps(inv.y.v,inv_det);
    inv.z.v=_mm_mul_ps(inv.z.v,inv_det);
    inv.w.v=_mm_mul_ps(inv.w.v,inv_det);

    return m44f_transpose(inv);
}

mat33f m33f_mul(mat33f l,mat33f r)
{
    r.x.v=_mm_add_ps(_mm_add_ps(
            _mm_mul_ps(l.x.v,_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r.x.v),0x00))),
            _mm_mul_ps(l.y.v,_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r.x.v),0x55)))),
            _mm_mul_ps(l.z.v,_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r.x.v),0xAA))));

    r.y.v=_mm_add_ps(_mm_add_ps(
            _mm_mul_ps(l.x.v,_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r.y.v),0x00))),
            _mm_mul_ps(l.y.v,_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r.y.v),0x55)))),
            _mm_mul_ps(l.z.v,_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r.y.v),0xAA))));

    r.z.v=_mm_add_ps(_mm_add_ps(
            _mm_mul_ps(l.x.v,_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r.z.v),0x00))),
            _mm_mul_ps(l.y.v,_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r.z.v),0x55)))),
            _mm_mul_ps(l.z.v,_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(r.z.v),0xAA))));

    return r;
}
mat33f m33f_inv(mat33f m)
{
    mat33f inv;

    inv.x.v=_mm_sub_ps(
        _mm_mul_ps(_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.y.v),0xC9)),_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.z.v),0xD2))),
        _mm_mul_ps(_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.y.v),0xD2)),_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.z.v),0xC9))));

    __m128 inv_det=_mm_dp_ps(m.x.v,inv.x.v,0x77);
    assert(_mm_cvtss_f32(inv_det)!=0.0f);///matrix is not invertible
    inv_det=_mm_div_ps(_mm_set1_ps(1.0f),inv_det);

    inv.y.v=_mm_sub_ps(
        _mm_mul_ps(_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.x.v),0xD2)),_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.z.v),0xC9))),
        _mm_mul_ps(_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.x.v),0xC9)),_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.z.v),0xD2))));

    inv.z.v=_mm_sub_ps(
        _mm_mul_ps(_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.x.v),0xC9)),_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.y.v),0xD2))),
        _mm_mul_ps(_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.x.v),0xD2)),_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.y.v),0xC9))));

    inv.x.v=_mm_mul_ps(inv.x.v,inv_det);
    inv.y.v=_mm_mul_ps(inv.y.v,inv_det);
    inv.z.v=_mm_mul_ps(inv.z.v,inv_det);

    return m33f_transpose(inv);
}
mat33f m33f_inv_det(mat33f m,float * determinant)
{
    mat33f inv;

    inv.x.v=_mm_sub_ps(
        _mm_mul_ps(_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.y.v),0xC9)),_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.z.v),0xD2))),
        _mm_mul_ps(_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.y.v),0xD2)),_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.z.v),0xC9))));

    __m128 inv_det=_mm_dp_ps(m.x.v,inv.x.v,0x77);
    *determinant=_mm_cvtss_f32(inv_det);
    inv_det=_mm_div_ps(_mm_set1_ps(1.0f),inv_det);

    inv.y.v=_mm_sub_ps(
        _mm_mul_ps(_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.x.v),0xD2)),_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.z.v),0xC9))),
        _mm_mul_ps(_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.x.v),0xC9)),_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.z.v),0xD2))));

    inv.z.v=_mm_sub_ps(
        _mm_mul_ps(_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.x.v),0xC9)),_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.y.v),0xD2))),
        _mm_mul_ps(_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.x.v),0xD2)),_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.y.v),0xC9))));

    inv.x.v=_mm_mul_ps(inv.x.v,inv_det);
    inv.y.v=_mm_mul_ps(inv.y.v,inv_det);
    inv.z.v=_mm_mul_ps(inv.z.v,inv_det);

    return m33f_transpose(inv);
}

#else
mat44f m44f_mul(mat44f l,mat44f r)
{
    mat44f result;

    result.x.x = r.x.x*l.x.x + r.x.y*l.y.x + r.x.z*l.z.x + r.x.w*l.w.x;
    result.x.y = r.x.x*l.x.y + r.x.y*l.y.y + r.x.z*l.z.y + r.x.w*l.w.y;
    result.x.z = r.x.x*l.x.z + r.x.y*l.y.z + r.x.z*l.z.z + r.x.w*l.w.z;
    result.x.w = r.x.x*l.x.w + r.x.y*l.y.w + r.x.z*l.z.w + r.x.w*l.w.w;

    result.y.x = r.y.x*l.x.x + r.y.y*l.y.x + r.y.z*l.z.x + r.y.w*l.w.x;
    result.y.y = r.y.x*l.x.y + r.y.y*l.y.y + r.y.z*l.z.y + r.y.w*l.w.y;
    result.y.z = r.y.x*l.x.z + r.y.y*l.y.z + r.y.z*l.z.z + r.y.w*l.w.z;
    result.y.w = r.y.x*l.x.w + r.y.y*l.y.w + r.y.z*l.z.w + r.y.w*l.w.w;

    result.z.x = r.z.x*l.x.x + r.z.y*l.y.x + r.z.z*l.z.x + r.z.w*l.w.x;
    result.z.y = r.z.x*l.x.y + r.z.y*l.y.y + r.z.z*l.z.y + r.z.w*l.w.y;
    result.z.z = r.z.x*l.x.z + r.z.y*l.y.z + r.z.z*l.z.z + r.z.w*l.w.z;
    result.z.w = r.z.x*l.x.w + r.z.y*l.y.w + r.z.z*l.z.w + r.z.w*l.w.w;

    result.w.x = r.w.x*l.x.x + r.w.y*l.y.x + r.w.z*l.z.x + r.w.w*l.w.x;
    result.w.y = r.w.x*l.x.y + r.w.y*l.y.y + r.w.z*l.z.y + r.w.w*l.w.y;
    result.w.z = r.w.x*l.x.z + r.w.y*l.y.z + r.w.z*l.z.z + r.w.w*l.w.z;
    result.w.w = r.w.x*l.x.w + r.w.y*l.y.w + r.w.z*l.z.w + r.w.w*l.w.w;

    return result;
}
mat44f m44f_inv(mat44f m)
{
    mat44f inv;
    float inv_det;

    inv.x.x=  m.y.y*m.z.z*m.w.w-m.y.y*m.z.w*m.w.z-m.z.y*m.y.z*m.w.w+m.z.y*m.y.w*m.w.z+m.w.y*m.y.z*m.z.w-m.w.y*m.y.w*m.z.z;
    inv.y.x= -m.y.x*m.z.z*m.w.w+m.y.x*m.z.w*m.w.z+m.z.x*m.y.z*m.w.w-m.z.x*m.y.w*m.w.z-m.w.x*m.y.z*m.z.w+m.w.x*m.y.w*m.z.z;
    inv.z.x=  m.y.x*m.z.y*m.w.w-m.y.x*m.z.w*m.w.y-m.z.x*m.y.y*m.w.w+m.z.x*m.y.w*m.w.y+m.w.x*m.y.y*m.z.w-m.w.x*m.y.w*m.z.y;
    inv.w.x= -m.y.x*m.z.y*m.w.z+m.y.x*m.z.z*m.w.y+m.z.x*m.y.y*m.w.z-m.z.x*m.y.z*m.w.y-m.w.x*m.y.y*m.z.z+m.w.x*m.y.z*m.z.y;

    inv.x.y= -m.x.y*m.z.z*m.w.w+m.x.y*m.z.w*m.w.z+m.z.y*m.x.z*m.w.w-m.z.y*m.x.w*m.w.z-m.w.y*m.x.z*m.z.w+m.w.y*m.x.w*m.z.z;
    inv.y.y=  m.x.x*m.z.z*m.w.w-m.x.x*m.z.w*m.w.z-m.z.x*m.x.z*m.w.w+m.z.x*m.x.w*m.w.z+m.w.x*m.x.z*m.z.w-m.w.x*m.x.w*m.z.z;
    inv.z.y= -m.x.x*m.z.y*m.w.w+m.x.x*m.z.w*m.w.y+m.z.x*m.x.y*m.w.w-m.z.x*m.x.w*m.w.y-m.w.x*m.x.y*m.z.w+m.w.x*m.x.w*m.z.y;
    inv.w.y=  m.x.x*m.z.y*m.w.z-m.x.x*m.z.z*m.w.y-m.z.x*m.x.y*m.w.z+m.z.x*m.x.z*m.w.y+m.w.x*m.x.y*m.z.z-m.w.x*m.x.z*m.z.y;

    inv.x.z=  m.x.y*m.y.z*m.w.w-m.x.y*m.y.w*m.w.z-m.y.y*m.x.z*m.w.w+m.y.y*m.x.w*m.w.z+m.w.y*m.x.z*m.y.w-m.w.y*m.x.w*m.y.z;
    inv.y.z= -m.x.x*m.y.z*m.w.w+m.x.x*m.y.w*m.w.z+m.y.x*m.x.z*m.w.w-m.y.x*m.x.w*m.w.z-m.w.x*m.x.z*m.y.w+m.w.x*m.x.w*m.y.z;
    inv.z.z=  m.x.x*m.y.y*m.w.w-m.x.x*m.y.w*m.w.y-m.y.x*m.x.y*m.w.w+m.y.x*m.x.w*m.w.y+m.w.x*m.x.y*m.y.w-m.w.x*m.x.w*m.y.y;
    inv.w.z= -m.x.x*m.y.y*m.w.z+m.x.x*m.y.z*m.w.y+m.y.x*m.x.y*m.w.z-m.y.x*m.x.z*m.w.y-m.w.x*m.x.y*m.y.z+m.w.x*m.x.z*m.y.y;

    inv.x.w= -m.x.y*m.y.z*m.z.w+m.x.y*m.y.w*m.z.z+m.y.y*m.x.z*m.z.w-m.y.y*m.x.w*m.z.z-m.z.y*m.x.z*m.y.w+m.z.y*m.x.w*m.y.z;
    inv.y.w=  m.x.x*m.y.z*m.z.w-m.x.x*m.y.w*m.z.z-m.y.x*m.x.z*m.z.w+m.y.x*m.x.w*m.z.z+m.z.x*m.x.z*m.y.w-m.z.x*m.x.w*m.y.z;
    inv.z.w= -m.x.x*m.y.y*m.z.w+m.x.x*m.y.w*m.z.y+m.y.x*m.x.y*m.z.w-m.y.x*m.x.w*m.z.y-m.z.x*m.x.y*m.y.w+m.z.x*m.x.w*m.y.y;
    inv.w.w=  m.x.x*m.y.y*m.z.z-m.x.x*m.y.z*m.z.y-m.y.x*m.x.y*m.z.z+m.y.x*m.x.z*m.z.y+m.z.x*m.x.y*m.y.z-m.z.x*m.x.z*m.y.y;

    inv_det=m.x.x*inv.x.x + m.x.y*inv.y.x + m.x.z * inv.z.x + m.x.w*inv.w.x;
    assert(inv_det!=0.0f);///matrix supplied is not invertible
    inv_det=1.0f/inv_det;

    ///can fail if det == 0

    inv.x.x*=inv_det;inv.x.y*=inv_det;inv.x.z*=inv_det;inv.x.w*=inv_det;
    inv.y.x*=inv_det;inv.y.y*=inv_det;inv.y.z*=inv_det;inv.y.w*=inv_det;
    inv.z.x*=inv_det;inv.z.y*=inv_det;inv.z.z*=inv_det;inv.z.w*=inv_det;
    inv.w.x*=inv_det;inv.w.y*=inv_det;inv.w.z*=inv_det;inv.w.w*=inv_det;

    return inv;
}

mat33f m33f_mul(mat33f l,mat33f r)
{
    mat33f result;

    result.x.x=l.x.x*r.x.x + l.y.x*r.x.y + l.z.x*r.x.z;
    result.x.y=l.x.y*r.x.x + l.y.y*r.x.y + l.z.y*r.x.z;
    result.x.z=l.x.z*r.x.x + l.y.z*r.x.y + l.z.z*r.x.z;

    result.y.x=l.x.x*r.y.x + l.y.x*r.y.y + l.z.x*r.y.z;
    result.y.y=l.x.y*r.y.x + l.y.y*r.y.y + l.z.y*r.y.z;
    result.y.z=l.x.z*r.y.x + l.y.z*r.y.y + l.z.z*r.y.z;

    result.z.x=l.x.x*r.z.x + l.y.x*r.z.y + l.z.x*r.z.z;
    result.z.y=l.x.y*r.z.x + l.y.y*r.z.y + l.z.y*r.z.z;
    result.z.z=l.x.z*r.z.x + l.y.z*r.z.y + l.z.z*r.z.z;

    return result;
}
mat33f m33f_inv(mat33f m)
{
    mat33f inv;
    float inv_det;

    inv.x.x=m.y.y*m.z.z-m.z.y*m.y.z;
    inv.x.y=m.z.y*m.x.z-m.x.y*m.z.z;
    inv.x.z=m.x.y*m.y.z-m.y.y*m.x.z;
    inv.y.x=m.z.x*m.y.z-m.y.x*m.z.z;
    inv.y.y=m.x.x*m.z.z-m.z.x*m.x.z;
    inv.y.z=m.y.x*m.x.z-m.x.x*m.y.z;
    inv.z.x=m.y.x*m.z.y-m.z.x*m.y.y;
    inv.z.y=m.z.x*m.x.y-m.x.x*m.z.y;
    inv.z.z=m.x.x*m.y.y-m.y.x*m.x.y;

    inv_det=1.0/(m.x.x*inv.x.x+m.x.y*inv.y.x+m.x.z*inv.z.x);

    inv.x.x*=inv_det;inv.x.y*=inv_det;inv.x.z*=inv_det;
    inv.y.x*=inv_det;inv.y.y*=inv_det;inv.y.z*=inv_det;
    inv.z.x*=inv_det;inv.z.y*=inv_det;inv.z.z*=inv_det;

    return inv;
}
mat33f m33f_inv_det(mat33f m,float * determinant)
{
    mat33f inv;
    float inv_det;

    inv.x.x=m.y.y*m.z.z-m.z.y*m.y.z;
    inv.x.y=m.z.y*m.x.z-m.x.y*m.z.z;
    inv.x.z=m.x.y*m.y.z-m.y.y*m.x.z;
    inv.y.x=m.z.x*m.y.z-m.y.x*m.z.z;
    inv.y.y=m.x.x*m.z.z-m.z.x*m.x.z;
    inv.y.z=m.y.x*m.x.z-m.x.x*m.y.z;
    inv.z.x=m.y.x*m.z.y-m.z.x*m.y.y;
    inv.z.y=m.z.x*m.x.y-m.x.x*m.z.y;
    inv.z.z=m.x.x*m.y.y-m.y.x*m.x.y;

    *determinant=m.x.x*inv.x.x+m.x.y*inv.y.x+m.x.z*inv.z.x;
    inv_det=1.0f/(*determinant);

    inv.x.x*=inv_det;inv.x.y*=inv_det;inv.x.z*=inv_det;
    inv.y.x*=inv_det;inv.y.y*=inv_det;inv.y.z*=inv_det;
    inv.z.x*=inv_det;inv.z.y*=inv_det;inv.z.z*=inv_det;

    return inv;
}
#endif












void m44f_print(mat44f m)
{
    printf("%f %f %f %f\n"  ,m.x.x,m.y.x,m.z.x,m.w.x);
    printf("%f %f %f %f\n"  ,m.x.y,m.y.y,m.z.y,m.w.y);
    printf("%f %f %f %f\n"  ,m.x.z,m.y.z,m.z.z,m.w.z);
    printf("%f %f %f %f\n\n",m.x.w,m.y.w,m.z.w,m.w.w);
}

void m33f_print(mat33f m)
{
    printf("%f %f %f\n"  ,m.x.x,m.y.x,m.z.x);
    printf("%f %f %f\n"  ,m.x.y,m.y.y,m.z.y);
    printf("%f %f %f\n\n",m.x.z,m.y.z,m.z.z);
}
