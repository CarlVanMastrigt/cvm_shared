/**
Copyright 2020,2021,2022,2023 Carl van Mastrigt

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

#include "cvm_shared.h"




//#define CVM_INTRINSIC_MODE_NONE
#define CVM_INTRINSIC_MODE_SSE /*this can trick IDE, when it doesn't pick up __SSE__ :p*/


///start with mmult, also probably want to rename, first thing is mat3x3 ??

#if (defined __SSE__ || defined CVM_INTRINSIC_MODE_SSE) && !defined CVM_INTRINSIC_MODE_NONE
mat44f m44f_transpose(mat44f m)
{
    ///names reflect columns-rows of respective vector entry
    __m128 xx_yx_xy_yy=_mm_unpacklo_ps(m.x.v,m.y.v);
    __m128 xz_yz_xw_yw=_mm_unpackhi_ps(m.x.v,m.y.v);

    __m128 zx_wx_zy_wy=_mm_unpacklo_ps(m.z.v,m.w.v);
    __m128 zz_wz_zw_ww=_mm_unpackhi_ps(m.z.v,m.w.v);

    m.x.v=_mm_movelh_ps(xx_yx_xy_yy,zx_wx_zy_wy);
    m.y.v=_mm_movehl_ps(zx_wx_zy_wy,xx_yx_xy_yy);
    m.z.v=_mm_movelh_ps(xz_yz_xw_yw,zz_wz_zw_ww);
    m.z.v=_mm_movehl_ps(zz_wz_zw_ww,xz_yz_xw_yw);

    return m;
}
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
    __m128 inv_det,ysp,ysn,zsp,zsn,wsp,wsn;///respective axies shifted posotively (+,p) or negatively (-,n) for the seubset of relevant entries
    mat44f inv;

    ///there is likely some way to optimise the swizzling performed such that values vectors can be recycled when calculating different columns

    ///probably also possible to optimise by replacing dp instructions with summedadditions of some description <---- almost certainly this
    ///     ^ accumulators of stacked shuffled multiplications...

    ///x not yet relevant for it's axis, and x components not relevant to following vectors
    ///note component swizzles done forward order, index numbers done reversed

    /// __y_z_w ; x-0's irrelevant
    ysp=_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.y.v),0x9C));// __w_y_z 2130
    ysn=_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.y.v),0x78));// __z_w_y 1320
    zsp=_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.z.v),0x9C));// __w_y_z 2130
    zsn=_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.z.v),0x78));// __z_w_y 1320
    wsp=_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.w.v),0x9C));// __w_y_z 2130
    wsn=_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.w.v),0x78));// __z_w_y 1320

    inv.x.v=_mm_blend_ps(
        _mm_blend_ps(
            _mm_dp_ps(m.y.v,_mm_sub_ps(_mm_mul_ps(zsn,wsp),_mm_mul_ps(zsp,wsn)),0xE1),
            _mm_dp_ps(m.x.v,_mm_sub_ps(_mm_mul_ps(zsp,wsn),_mm_mul_ps(zsn,wsp)),0xE2),
            0x02),
        _mm_blend_ps(
            _mm_dp_ps(m.x.v,_mm_sub_ps(_mm_mul_ps(ysn,wsp),_mm_mul_ps(ysp,wsn)),0xE4),
            _mm_dp_ps(m.x.v,_mm_sub_ps(_mm_mul_ps(ysp,zsn),_mm_mul_ps(ysn,zsp)),0xE8),
            0x08),
        0x0C);


    /// x___z_w ; y-1's irrelevant
    ysp=_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.y.v),0x87));// w___x_z 2013
    ysn=_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.y.v),0x36));// z___w_x 0312
    zsp=_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.z.v),0x87));// w___x_z 2013
    zsn=_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.z.v),0x36));// z___w_x 0312
    wsp=_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.w.v),0x87));// w___x_z 2013
    wsn=_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.w.v),0x36));// z___w_x 0312

    inv.y.v=_mm_blend_ps(
        _mm_blend_ps(
            _mm_dp_ps(m.y.v,_mm_sub_ps(_mm_mul_ps(zsp,wsn),_mm_mul_ps(zsn,wsp)),0xD1),
            _mm_dp_ps(m.x.v,_mm_sub_ps(_mm_mul_ps(zsn,wsp),_mm_mul_ps(zsp,wsn)),0xD2),
            0x02),
        _mm_blend_ps(
            _mm_dp_ps(m.x.v,_mm_sub_ps(_mm_mul_ps(ysp,wsn),_mm_mul_ps(ysn,wsp)),0xD4),
            _mm_dp_ps(m.x.v,_mm_sub_ps(_mm_mul_ps(ysn,zsp),_mm_mul_ps(ysp,zsn)),0xD8),
            0x08),
        0x0C);


    /// x_y___w ; z-2's irrelevant
    ysp=_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.y.v),0x63));// w_x___y 1203
    ysn=_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.y.v),0x2D));// y_w___x 0231
    zsp=_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.z.v),0x63));// w_x___y 1203
    zsn=_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.z.v),0x2D));// y_w___x 0231
    wsp=_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.w.v),0x63));// w_x___y 1203
    wsn=_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.w.v),0x2D));// y_w___x 0231

    inv.z.v=_mm_blend_ps(
        _mm_blend_ps(
            _mm_dp_ps(m.y.v,_mm_sub_ps(_mm_mul_ps(zsn,wsp),_mm_mul_ps(zsp,wsn)),0xB1),
            _mm_dp_ps(m.x.v,_mm_sub_ps(_mm_mul_ps(zsp,wsn),_mm_mul_ps(zsn,wsp)),0xB2),
            0x02),
        _mm_blend_ps(
            _mm_dp_ps(m.x.v,_mm_sub_ps(_mm_mul_ps(ysn,wsp),_mm_mul_ps(ysp,wsn)),0xB4),
            _mm_dp_ps(m.x.v,_mm_sub_ps(_mm_mul_ps(ysp,zsn),_mm_mul_ps(ysn,zsp)),0xB8),
            0x08),
        0x0C);


    /// x_y_z__ ; w-3's irrelevant
    ysp=_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.y.v),0xD2));// z_x_y__ 3102
    ysn=_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.y.v),0xC9));// y_z_x__ 3021
    zsp=_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.z.v),0xD2));// z_x_y__ 3102
    zsn=_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.z.v),0xC9));// y_z_x__ 3021
    wsp=_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.w.v),0xD2));// z_x_y__ 3102
    wsn=_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.w.v),0xC9));// y_z_x__ 3021

    inv.w.v=_mm_blend_ps(
        _mm_blend_ps(
            _mm_dp_ps(m.y.v,_mm_sub_ps(_mm_mul_ps(zsp,wsn),_mm_mul_ps(zsn,wsp)),0x71),
            _mm_dp_ps(m.x.v,_mm_sub_ps(_mm_mul_ps(zsn,wsp),_mm_mul_ps(zsp,wsn)),0x72),
            0x02),
        _mm_blend_ps(
            _mm_dp_ps(m.x.v,_mm_sub_ps(_mm_mul_ps(ysp,wsn),_mm_mul_ps(ysn,wsp)),0x74),
            _mm_dp_ps(m.x.v,_mm_sub_ps(_mm_mul_ps(ysn,zsp),_mm_mul_ps(ysp,zsn)),0x78),
            0x08),
        0x0C);


    inv_det=_mm_dp_ps(m.x.v,_mm_movelh_ps(_mm_unpacklo_ps(inv.x.v,inv.y.v),_mm_unpacklo_ps(inv.z.v,inv.w.v)),0xFF);
    assert(_mm_cvtss_f32(inv_det)!=0.0f);///matrix is not invertible
    inv_det=_mm_div_ps(_mm_set1_ps(1.0f),inv_det);

    inv.x.v=_mm_mul_ps(inv.x.v,inv_det);
    inv.y.v=_mm_mul_ps(inv.y.v,inv_det);
    inv.z.v=_mm_mul_ps(inv.z.v,inv_det);
    inv.w.v=_mm_mul_ps(inv.w.v,inv_det);

    return inv;
}
vec4f m44f_v4f_mul(mat44f m,vec4f v)
{
    return (vec4f){.v=_mm_add_ps(
        _mm_add_ps(
            _mm_mul_ps(m.x.v,_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(v.v),0x00))),
            _mm_mul_ps(m.y.v,_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(v.v),0x55)))),
        _mm_add_ps(
            _mm_mul_ps(m.z.v,_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(v.v),0xAA))),
            _mm_mul_ps(m.w.v,_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(v.v),0xFF)))))};
}
vec3f m44f_v4f_mul_p(mat44f m,vec4f v)
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
#else
mat44f m44f_transpose(mat44f m)
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
mat44f m44f_mul(mat44f l,mat44f r)
{
    matrix4f result;

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
    matrix4f inv;
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
vec4f m44f_v4f_mul(mat44f m,vec4f v)
{
    return (vec4f){ .x=m.x.x*v.x + m.y.x*v.y + m.z.x*v.z + m.w.x*v.w,
                    .y=m.x.y*v.x + m.y.y*v.y + m.z.y*v.z + m.w.y*v.w,
                    .z=m.x.z*v.x + m.y.z*v.y + m.z.z*v.z + m.w.z*v.w,
                    .w=m.x.w*v.x + m.y.w*v.y + m.z.w*v.z + m.w.w*v.w};
}
vec3f m44f_v4f_mul_p(mat44f m,vec4f v)
{
    return v3f_div(
    (vec3f){.x=m.x.x*v.x + m.y.x*v.y + m.z.x*v.z + m.w.x*v.w,
            .y=m.x.y*v.x + m.y.y*v.y + m.z.y*v.z + m.w.y*v.w,
            .z=m.x.z*v.x + m.y.z*v.y + m.z.z*v.z + m.w.z*v.w},
               m.x.w*v.x + m.y.w*v.y + m.z.w*v.z + m.w.w*v.w);
}
#endif









matrix4f m4f_mult(matrix4f l,matrix4f r)
{
    return m44f_mul(l,r);
}


matrix4f m4f_inv(matrix4f m)
{
    return m44f_inv(m);
}

vec3f m4f_v4f_mult_p(matrix4f m,vec4f v)
{
    return m44f_v4f_mul_p(m,v);
}









matrix3f m3f_inv(matrix3f m)
{
    assert(false);///needs to be converted to vector/intrinsic ops, though really would to prefer not using m3x3
    matrix3f inv;
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

matrix3f m3f_inv_det(matrix3f m,float * determinant)
{
    assert(false);///needs to be converted to vector/intrinsic ops, though really would to prefer not using m3x3
    matrix3f inv;
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

vec3f m3f_v3f_mult(matrix3f m,vec3f v)
{
    assert(false);///needs to be converted to vector/intrinsic ops, though really would to prefer not using m3x3
    vec3f result;

    result.x=m.x.x*v.x + m.y.x*v.y + m.z.x*v.z;
    result.y=m.x.y*v.x + m.y.y*v.y + m.z.y*v.z;
    result.z=m.x.z*v.x + m.y.z*v.y + m.z.z*v.z;

    return result;
}

matrix3f m3f_mult(matrix3f l,matrix3f r)
{
    assert(false);///needs to be converted to vector/intrinsic ops, though really would to prefer not using m3x3
    matrix3f result;

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

