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
    vec2_s16 start;
    vec2_s16 end;
}
rect_s16;

static inline rect_s16 rect_s16_intersect(rect_s16 r1, rect_s16 r2)
{
	/// this doesnt work with inverted rects
    r1.start.x += (r2.start.x > r1.start.x) ? (r2.start.x - r1.start.x) : 0;
    r1.start.y += (r2.start.y > r1.start.y) ? (r2.start.y - r1.start.y) : 0;

    r1.end.x += (r2.end.x < r1.end.x) ? (r2.end.x - r1.end.x) : 0;
    r1.end.y += (r2.end.y < r1.end.y) ? (r2.end.y - r1.end.y) : 0;

    return r1;
}
static inline bool rect_s16_positive_area(rect_s16 r)
{
    return r.end.x > r.start.x && r.end.y > r.start.y;
}
static inline rect_s16 rect_s16_add_offset(rect_s16 r, vec2_s16 o)
{
    return (rect_s16){.start.x=r.start.x+o.x, .start.y=r.start.y+o.y, .end.x=r.end.x+o.x, .end.y=r.end.y+o.y};
}
static inline rect_s16 rect_s16_sub_offset(rect_s16 r, vec2_s16 o)
{
    return (rect_s16){.start.x=r.start.x-o.x, .start.y=r.start.y-o.y,.end.x=r.end.x-o.x, .end.y=r.end.y-o.y};
}
static inline bool rect_s16_contains_point(rect_s16 r, vec2_s16 p)
{
	/// this doesnt work with inverted rects
    return ((r.start.x <= p.x)&&(r.start.y <= p.y)&&(r.end.x > p.x)&&(r.end.y> p.y));
}
static inline bool rect_s16_contains_origin(rect_s16 r)
{
	/// this doesnt work with inverted rects
    return ((r.start.x <= 0)&&(r.start.y <= 0)&&(r.end.x > 0)&&(r.end.y> 0));
}
static inline bool rect_s16_have_overlap(rect_s16 lhs, rect_s16 rhs)
{
	/// this doesnt work with inverted rects
    return lhs.start.x<rhs.end.x && rhs.start.x<lhs.end.x && lhs.start.y<rhs.end.y && rhs.start.y<lhs.end.y;
}