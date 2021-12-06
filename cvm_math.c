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

#include "cvm_shared.h"

matrix4f m4f_mult(matrix4f l,matrix4f r)
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

matrix4f m4f_inv(matrix4f m)
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

    inv_det=1.0f/(m.x.x*inv.x.x + m.x.y*inv.y.x + m.x.z * inv.z.x + m.x.w*inv.w.x);

    ///can fail if det == 0

    inv.x.x*=inv_det;inv.x.y*=inv_det;inv.x.z*=inv_det;inv.x.w*=inv_det;
    inv.y.x*=inv_det;inv.y.y*=inv_det;inv.y.z*=inv_det;inv.y.w*=inv_det;
    inv.z.x*=inv_det;inv.z.y*=inv_det;inv.z.z*=inv_det;inv.z.w*=inv_det;
    inv.w.x*=inv_det;inv.w.y*=inv_det;inv.w.z*=inv_det;inv.w.w*=inv_det;

    return inv;
}

vec3f m4f_v4f_mult_p(matrix4f m,vec4f v)
{
    return v3f_div(
        (vec3f){.x=m.x.x*v.x + m.y.x*v.y + m.z.x*v.z + m.w.x*v.w,
                .y=m.x.y*v.x + m.y.y*v.y + m.z.y*v.z + m.w.y*v.w,
                .z=m.x.z*v.x + m.y.z*v.y + m.z.z*v.z + m.w.z*v.w},
                   m.x.w*v.x + m.y.w*v.y + m.z.w*v.z + m.w.w*v.w);
}









matrix3f m3f_inv(matrix3f m)
{
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
    vec3f result;

    result.x=m.x.x*v.x + m.y.x*v.y + m.z.x*v.z;
    result.y=m.x.y*v.x + m.y.y*v.y + m.z.y*v.z;
    result.z=m.x.z*v.x + m.y.z*v.y + m.z.z*v.z;

    return result;
}

matrix3f m3f_mult(matrix3f l,matrix3f r)
{
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

