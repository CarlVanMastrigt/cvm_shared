/**
Copyright 2025 Carl van Mastrigt

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

#pragma once

#include <inttypes.h>
#include <stdbool.h>

#include "math/vec2_s16.h"


typedef struct rect_s16
{
    int16_t x1;
    int16_t y1;
    int16_t x2;
    int16_t y2;
}
rect_s16;

///rectangle
static inline rect_s16 rect_s16_set(int16_t x1,int16_t y1,int16_t x2,int16_t y2)
{
    return (rect_s16){.x1=x1, .y1=y1, .x2=x2, .y2=y2};
}
static inline rect_s16 rect_s16_intersect(rect_s16 r1, rect_s16 r2)
{
	/// this doesnt work with inverted rects
    r1.x1 += (r2.x1 > r1.x1) ? (r2.x1 - r1.x1) : 0;
    r1.y1 += (r2.y1 > r1.y1) ? (r2.y1 - r1.y1) : 0;

    r1.x2 += (r2.x2 < r1.x2) ? (r2.x2 - r1.x2) : 0;
    r1.y2 += (r2.y2 < r1.y2) ? (r2.y2 - r1.y2) : 0;

    return r1;
}
static inline bool rect_s16_positive_area(rect_s16 r)
{
    return r.x2 > r.x1 && r.y2 > r.y1;
}
static inline rect_s16 rect_s16_add_offset(rect_s16 r, vec2_s16 o)
{
    return (rect_s16){.x1=r.x1+o.x, .y1=r.y1+o.y, .x2=r.x2+o.x, .y2=r.y2+o.y};
}
static inline rect_s16 rect_s16_sub_offset(rect_s16 r, vec2_s16 o)
{
    return (rect_s16){.x1=r.x1-o.x, .y1=r.y1-o.y,.x2=r.x2-o.x, .y2=r.y2-o.y};
}
static inline bool rect_s16_contains_point(rect_s16 r, vec2_s16 p)
{
	/// this doesnt work with inverted rects
    return ((r.x1 <= p.x)&&(r.y1 <= p.y)&&(r.x2 > p.x)&&(r.y2> p.y));
}
static inline bool rect_s16_contains_origin(rect_s16 r)
{
	/// this doesnt work with inverted rects
    return ((r.x1 <= 0)&&(r.y1 <= 0)&&(r.x2 > 0)&&(r.y2> 0));
}
static inline bool rect_s16_have_overlap(rect_s16 lhs, rect_s16 rhs)
{
	/// this doesnt work with inverted rects
    return lhs.x1<rhs.x2 && rhs.x1<lhs.x2 && lhs.y1<rhs.y2 && rhs.y1<lhs.y2;
}